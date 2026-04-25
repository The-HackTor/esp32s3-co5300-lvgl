/* Stub NFC backend for the ESP-IDF port. All calls log WARN and either
 * return canned data immediately or fire a deferred callback via esp_timer so
 * the UI has time to show its "reading..." animation. */

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

void hw_nfc_init(bool nfc_available)
{
    s_nfc_available = nfc_available;
    ESP_LOGI(TAG, "stub init (avail=%d) — all reads/attacks return canned data",
             nfc_available);
}

void hw_nfc_generate_fake_dump(struct mfc_dump *dump)
{
    if (!dump) return;
    memset(dump, 0, sizeof(*dump));
    /* Synthesize a plausible 1K MIFARE Classic dump */
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
    /* Pretend we found all key A's */
    dump->key_a_mask = (1ULL << MFC_1K_SECTORS) - 1;
    dump->key_b_mask = 0;
    for (int s = 0; s < MFC_1K_SECTORS; s++) {
        memset(dump->key_a[s], 0xFF, 6);  /* canonical default */
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
    if (d->mfc_out) {
        hw_nfc_generate_fake_dump(d->mfc_out);
        if (d->cb) d->cb(true, NFC_CARD_MFC, d->ctx);
    } else if (d->mfu_out) {
        generate_fake_mfu(d->mfu_out);
        if (d->cb) d->cb(true, NFC_CARD_MFU, d->ctx);
    } else {
        if (d->cb) d->cb(false, NFC_CARD_NONE, d->ctx);
    }
    esp_timer_delete(d->timer);
    free(d);
}

static struct read_deferred *s_pending_read;

void hw_nfc_start_read(struct mfc_dump *mfc_out, struct mfu_dump *mfu_out,
                       hw_nfc_read_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_read (defer %d ms)", DEFER_MS_READ);
    struct read_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    d->mfc_out = mfc_out;
    d->mfu_out = mfu_out;
    d->cb      = cb;
    d->ctx     = ctx;
    const esp_timer_create_args_t args = {
        .callback = read_timer_cb, .arg = d, .name = "nfc_read_stub",
    };
    esp_timer_create(&args, &d->timer);
    s_pending_read = d;
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_READ * 1000);
}

void hw_nfc_cancel_read(void)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_cancel_read");
    if (s_pending_read && s_pending_read->timer) {
        esp_timer_stop(s_pending_read->timer);
        esp_timer_delete(s_pending_read->timer);
        free(s_pending_read);
        s_pending_read = NULL;
    }
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
    struct mfkey32_result fake = {
        .found = true,
        .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .elapsed_ms = 2500,
    };
    if (d->cb) d->cb(&fake, 1, d->ctx);
    esp_timer_delete(d->timer);
    free(d);
}

void hw_nfc_solve_mfkey32(const struct mfkey32_nonce_pair *pairs, size_t count,
                          hw_nfc_mfkey32_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_solve_mfkey32 (count=%u)", (unsigned)count);
    struct mfkey32_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    d->pairs = pairs; d->count = count; d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = mfkey32_timer_cb, .arg = d, .name = "nfc_mfkey_stub",
    };
    esp_timer_create(&args, &d->timer);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}

static void nested_timer_cb(void *arg)
{
    struct nested_deferred *d = (struct nested_deferred *)arg;
    struct nested_result fake = {
        .found = true,
        .key = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},
        .prng_type = NESTED_PRNG_WEAK,
        .elapsed_ms = 1800,
    };
    if (d->cb) d->cb(true, &fake, d->ctx);
    esp_timer_delete(d->timer);
    free(d);
}

void hw_nfc_start_nested(const struct nested_params *params,
                         hw_nfc_nested_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_nested");
    struct nested_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    if (params) d->params = *params;
    d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = nested_timer_cb, .arg = d, .name = "nfc_nested_stub",
    };
    esp_timer_create(&args, &d->timer);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}

static void hardnested_timer_cb(void *arg)
{
    struct hardnested_deferred *d = (struct hardnested_deferred *)arg;
    struct hardnested_result fake = {
        .found = true,
        .key = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5},
        .nonces_collected = 4096,
        .elapsed_ms = 5200,
    };
    if (d->cb) d->cb(true, &fake, d->ctx);
    esp_timer_delete(d->timer);
    free(d);
}

void hw_nfc_start_hardnested(const struct nested_params *params,
                             hw_nfc_hardnested_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: hw_nfc_start_hardnested");
    struct hardnested_deferred *d = calloc(1, sizeof(*d));
    if (!d) return;
    if (params) d->params = *params;
    d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = hardnested_timer_cb, .arg = d, .name = "nfc_hardn_stub",
    };
    esp_timer_create(&args, &d->timer);
    esp_timer_start_once(d->timer, (uint64_t)DEFER_MS_CRYPTO * 1000);
}
