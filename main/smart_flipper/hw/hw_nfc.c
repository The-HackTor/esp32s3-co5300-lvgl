/* Stub NFC backend: canned data fired through esp_timer to keep UI animations honest.
 *
 * Async-slot lifecycle invariant (applied to every s_pending_* tracker below):
 *   one-shot fire: detach s_X -> snapshot locals -> free struct -> user cb ->
 *                  delete timer last (esp_timer needs the handle valid until
 *                  the cb returns).
 *   cancel:        atomically claim s_X; if non-NULL, stop+delete timer, free.
 * Tracker accesses use __atomic_* so the LVGL task and the esp_timer task
 * don't tear across the dual-core boundary. */

#include "hw_nfc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "hw_nfc";
static bool s_nfc_available;
static bool s_emulating;

#define DEFER_MS_READ     600
#define DEFER_MS_CRYPTO   300

#define CLAIM(p)       __atomic_exchange_n(&(p), NULL, __ATOMIC_ACQ_REL)
#define DETACH(p, me)  do { __typeof__(p) _e = (me); \
    __atomic_compare_exchange_n(&(p), &_e, NULL, false, \
                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); } while(0)

struct read_deferred {
    struct mfc_dump       *mfc_out;
    struct mfu_dump       *mfu_out;
    hw_nfc_read_cb_t       cb;
    void                  *ctx;
    esp_timer_handle_t     timer;
};

struct mfkey32_deferred {
    const struct mfkey32_nonce_pair *pairs;
    size_t                            count;
    hw_nfc_mfkey32_cb_t               cb;
    void                             *ctx;
    esp_timer_handle_t                timer;
};

struct nested_deferred {
    struct nested_params        params;
    hw_nfc_nested_cb_t          cb;
    void                       *ctx;
    esp_timer_handle_t          timer;
};

struct hardnested_deferred {
    struct nested_params             params;
    hw_nfc_hardnested_cb_t           cb;
    void                            *ctx;
    esp_timer_handle_t               timer;
};

static struct read_deferred       *s_pending_read;
static struct mfkey32_deferred    *s_pending_mfkey32;
static struct nested_deferred     *s_pending_nested;
static struct hardnested_deferred *s_pending_hardnested;

void hw_nfc_init(bool nfc_available)
{
    s_nfc_available = nfc_available;
    ESP_LOGI(TAG, "stub init (avail=%d)", nfc_available);
}

void hw_nfc_generate_fake_dump(struct mfc_dump *dump)
{
    if (!dump) return;
    memset(dump, 0, sizeof(*dump));
    static const uint8_t uid[] = {0xDE, 0xAD, 0xBE, 0xEF};
    memcpy(dump->card.uid, uid, sizeof(uid));
    dump->card.uid_len = sizeof(uid);
    dump->card.sak = ISO14443A_SAK_MFC_1K;
    dump->card.atqa[0] = 0x04;
    dump->card.atqa[1] = 0x00;
    dump->type = MFC_TYPE_1K;
    for (int b = 0; b < MFC_1K_BLOCKS; b++) {
        dump->block_read[b] = true;
        for (int i = 0; i < MFC_BLOCK_SIZE; i++) {
            dump->blocks[b][i] = (uint8_t)(b * 16 + i);
        }
    }
    dump->key_a_mask = (1ULL << MFC_1K_SECTORS) - 1;
    dump->key_b_mask = 0;
    for (int s = 0; s < MFC_1K_SECTORS; s++) {
        memset(dump->key_a[s], 0xFF, 6);
    }
    dump->read_time_ms = 450;
}

static void generate_fake_mfu(struct mfu_dump *dump)
{
    if (!dump) return;
    memset(dump, 0, sizeof(*dump));
    static const uint8_t uid[] = {0x04, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    memcpy(dump->card.uid, uid, sizeof(uid));
    dump->card.uid_len = sizeof(uid);
    dump->card.sak = 0x00;
    dump->card.atqa[0] = 0x44;
    dump->type = MFU_TYPE_NTAG215;
    dump->total_pages = 135;
    for (uint16_t p = 0; p < dump->total_pages; p++) {
        dump->page_read[p] = true;
        for (int i = 0; i < MFU_PAGE_SIZE; i++) {
            dump->pages[p][i] = (uint8_t)((p << 2) | i);
        }
    }
    dump->read_time_ms = 120;
}

static void read_timer_cb(void *arg)
{
    struct read_deferred *d = (struct read_deferred *)arg;
    DETACH(s_pending_read, d);

    bool                 success = false;
    enum nfc_card_type   type    = NFC_CARD_NONE;
    if (d->mfc_out) {
        hw_nfc_generate_fake_dump(d->mfc_out);
        success = true; type = NFC_CARD_MFC;
    } else if (d->mfu_out) {
        generate_fake_mfu(d->mfu_out);
        success = true; type = NFC_CARD_MFU;
    }
    hw_nfc_read_cb_t   cb  = d->cb;
    void              *ctx = d->ctx;
    esp_timer_handle_t t   = d->timer;
    free(d);
    if (cb) cb(success, type, ctx);
    esp_timer_delete(t);
}

void hw_nfc_start_read(struct mfc_dump *mfc_out, struct mfu_dump *mfu_out,
                       hw_nfc_read_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_read (defer %d ms)", DEFER_MS_READ);
    hw_nfc_cancel_read();
    struct read_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    d->mfc_out = mfc_out;
    d->mfu_out = mfu_out;
    d->cb      = cb;
    d->ctx     = ctx;
    const esp_timer_create_args_t args = {
        .callback = read_timer_cb, .arg = d, .name = "nfc_read_stub",
    };
    if (esp_timer_create(&args, &d->timer) != ESP_OK) { free(d); return; }
    __atomic_store_n(&s_pending_read, d, __ATOMIC_RELEASE);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_READ * 1000);
}

void hw_nfc_cancel_read(void)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_cancel_read");
    struct read_deferred *d = CLAIM(s_pending_read);
    if (!d) return;
    esp_timer_stop(d->timer);
    esp_timer_delete(d->timer);
    free(d);
}

void hw_nfc_start_emulate(struct mfc_dump *src, hw_nfc_nonce_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_emulate (MFC)");
    (void)src; (void)cb; (void)ctx;
    s_emulating = true;
}

void hw_nfc_start_emulate_mfu(struct mfu_dump *src)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_emulate_mfu");
    (void)src;
    s_emulating = true;
}

void hw_nfc_stop_emulate(void)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_stop_emulate");
    s_emulating = false;
}

bool hw_nfc_is_emulating(void)
{
    return s_emulating;
}

static void mfkey32_timer_cb(void *arg)
{
    struct mfkey32_deferred *d = (struct mfkey32_deferred *)arg;
    DETACH(s_pending_mfkey32, d);

    struct mfkey32_result fake = {
        .found = true,
        .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .elapsed_ms = 2500,
    };
    hw_nfc_mfkey32_cb_t cb  = d->cb;
    void               *ctx = d->ctx;
    esp_timer_handle_t  t   = d->timer;
    free(d);
    if (cb) cb(&fake, 1, ctx);
    esp_timer_delete(t);
}

void hw_nfc_solve_mfkey32(const struct mfkey32_nonce_pair *pairs, size_t count,
                          hw_nfc_mfkey32_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_solve_mfkey32 (count=%u)", (unsigned)count);
    struct mfkey32_deferred *prev = CLAIM(s_pending_mfkey32);
    if (prev) {
        esp_timer_stop(prev->timer);
        esp_timer_delete(prev->timer);
        free(prev);
    }
    struct mfkey32_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    d->pairs = pairs; d->count = count; d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = mfkey32_timer_cb, .arg = d, .name = "nfc_mfkey_stub",
    };
    if (esp_timer_create(&args, &d->timer) != ESP_OK) { free(d); return; }
    __atomic_store_n(&s_pending_mfkey32, d, __ATOMIC_RELEASE);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}

static void nested_timer_cb(void *arg)
{
    struct nested_deferred *d = (struct nested_deferred *)arg;
    DETACH(s_pending_nested, d);

    struct nested_result fake = {
        .found = true,
        .key = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
        .prng_type = NESTED_PRNG_WEAK,
        .elapsed_ms = 1800,
    };
    hw_nfc_nested_cb_t cb  = d->cb;
    void              *ctx = d->ctx;
    esp_timer_handle_t t   = d->timer;
    free(d);
    if (cb) cb(true, &fake, ctx);
    esp_timer_delete(t);
}

void hw_nfc_start_nested(const struct nested_params *params,
                         hw_nfc_nested_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_nested");
    struct nested_deferred *prev = CLAIM(s_pending_nested);
    if (prev) {
        esp_timer_stop(prev->timer);
        esp_timer_delete(prev->timer);
        free(prev);
    }
    struct nested_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    if (params) d->params = *params;
    d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = nested_timer_cb, .arg = d, .name = "nfc_nested_stub",
    };
    if (esp_timer_create(&args, &d->timer) != ESP_OK) { free(d); return; }
    __atomic_store_n(&s_pending_nested, d, __ATOMIC_RELEASE);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}

static void hardnested_timer_cb(void *arg)
{
    struct hardnested_deferred *d = (struct hardnested_deferred *)arg;
    DETACH(s_pending_hardnested, d);

    struct hardnested_result fake = {
        .found = true,
        .key = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
        .nonces_collected = 4096,
        .elapsed_ms = 5200,
    };
    hw_nfc_hardnested_cb_t cb  = d->cb;
    void                  *ctx = d->ctx;
    esp_timer_handle_t     t   = d->timer;
    free(d);
    if (cb) cb(true, &fake, ctx);
    esp_timer_delete(t);
}

void hw_nfc_start_hardnested(const struct nested_params *params,
                             hw_nfc_hardnested_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_hardnested");
    struct hardnested_deferred *prev = CLAIM(s_pending_hardnested);
    if (prev) {
        esp_timer_stop(prev->timer);
        esp_timer_delete(prev->timer);
        free(prev);
    }
    struct hardnested_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    if (params) d->params = *params;
    d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = hardnested_timer_cb, .arg = d, .name = "nfc_hardn_stub",
    };
    if (esp_timer_create(&args, &d->timer) != ESP_OK) { free(d); return; }
    __atomic_store_n(&s_pending_hardnested, d, __ATOMIC_RELEASE);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}
