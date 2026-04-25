/* Stub SubGHz backend for the ESP-IDF port. Synthesizes canned data so the UI
 * flow (analyzer, capture, replay, decode, ping, relay) is exercisable end to
 * end without the radio. */

#include "hw_subghz.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "hw_subghz";

static bool     s_available;
static bool     s_capturing;
static bool     s_hopping;
static bool     s_hop_locked;
static uint32_t s_hop_current_khz = 433920;
static uint32_t s_frequency_khz   = 433920;
static uint8_t  s_preset          = 0;

static SimSubghzRaw      s_last_raw;
static SimSubghzDecoded  s_last_decoded;
static SubghzHistoryEntry s_history[SUBGHZ_HISTORY_MAX];
static uint16_t          s_history_count;

static struct mfc_dump   s_relay_dump;

#define DEFER_MS_SHORT  200
#define DEFER_MS_MED    500
#define DEFER_MS_LONG   1200

void hw_subghz_init(const struct device *radio, bool radio_available)
{
    (void)radio;
    s_available = radio_available;
    memset(&s_last_raw, 0, sizeof(s_last_raw));
    memset(&s_last_decoded, 0, sizeof(s_last_decoded));
    memset(s_history, 0, sizeof(s_history));
    s_history_count = 0;
    ESP_LOGI(TAG, "stub init (avail=%d) — synthesized data", radio_available);
}

/* -------- Analyzer -------- */

struct analyzer_ctx {
    hw_subghz_analyzer_cb_t cb;
    void                   *ctx;
    esp_timer_handle_t      timer;
    uint32_t                tick;
};

static struct analyzer_ctx *s_az;

static void analyzer_tick(void *arg)
{
    struct analyzer_ctx *a = (struct analyzer_ctx *)arg;
    if (!a->cb) return;
    SimSubghzFreqResult r = {
        .freq_khz = 433920 + ((a->tick % 40) - 20) * 10,
        .rssi     = -60.0f - (float)(a->tick % 30),
    };
    a->cb(&r, a->ctx);
    a->tick++;
}

void hw_subghz_start_analyzer(hw_subghz_analyzer_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_analyzer");
    hw_subghz_stop_analyzer();
    s_az = calloc(1, sizeof(*s_az));
    if (!s_az) return;
    s_az->cb = cb; s_az->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = analyzer_tick, .arg = s_az, .name = "az_stub",
    };
    esp_timer_create(&args, &s_az->timer);
    esp_timer_start_periodic(s_az->timer, 100 * 1000);
}

void hw_subghz_stop_analyzer(void)
{
    ESP_LOGW(TAG, "STUB: stop_analyzer");
    if (!s_az) return;
    if (s_az->timer) {
        esp_timer_stop(s_az->timer);
        esp_timer_delete(s_az->timer);
    }
    free(s_az);
    s_az = NULL;
}

/* -------- Capture -------- */

struct capture_ctx {
    hw_subghz_capture_cb_t cb;
    void                  *ctx;
    esp_timer_handle_t     timer;
};

static struct capture_ctx *s_cap;

static void capture_fire(void *arg)
{
    struct capture_ctx *c = (struct capture_ctx *)arg;
    s_last_raw.count = 16;
    s_last_raw.full_count = 16;
    for (int i = 0; i < 16; i++) {
        s_last_raw.samples[i] = (i & 1) ? 450 : -450;
    }
    s_last_decoded.protocol = "Princeton";
    s_last_decoded.data = 0xA5A5A5ULL;
    s_last_decoded.bits = 24;
    s_last_decoded.te   = 400;
    s_capturing = false;
    if (c->cb) c->cb(true, &s_last_raw, c->ctx);
    esp_timer_delete(c->timer);
    free(c);
    s_cap = NULL;
}

static void kick_capture_defer(hw_subghz_capture_cb_t cb, void *ctx, uint32_t ms)
{
    hw_subghz_stop_capture();
    s_cap = calloc(1, sizeof(*s_cap));
    if (!s_cap) return;
    s_cap->cb = cb; s_cap->ctx = ctx;
    s_capturing = true;
    const esp_timer_create_args_t args = {
        .callback = capture_fire, .arg = s_cap, .name = "cap_stub",
    };
    esp_timer_create(&args, &s_cap->timer);
    esp_timer_start_once(s_cap->timer, (uint64_t)ms * 1000);
}

void hw_subghz_start_capture(hw_subghz_capture_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_capture");
    kick_capture_defer(cb, ctx, DEFER_MS_LONG);
}

void hw_subghz_start_capture_ex(uint32_t duration_ms, float rssi_threshold,
                                hw_subghz_capture_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_capture_ex (dur=%u, thr=%.1f)",
             (unsigned)duration_ms, rssi_threshold);
    kick_capture_defer(cb, ctx, duration_ms ? duration_ms : DEFER_MS_LONG);
}

void hw_subghz_start_capture_continuous(float rssi_threshold,
                                        hw_subghz_capture_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_capture_continuous (thr=%.1f)", rssi_threshold);
    kick_capture_defer(cb, ctx, DEFER_MS_LONG);
}

void hw_subghz_stop_capture(void)
{
    ESP_LOGW(TAG, "STUB: stop_capture");
    if (s_cap) {
        if (s_cap->timer) {
            esp_timer_stop(s_cap->timer);
            esp_timer_delete(s_cap->timer);
        }
        free(s_cap);
        s_cap = NULL;
    }
    s_capturing = false;
}

bool hw_subghz_is_capturing(void)          { return s_capturing; }
float hw_subghz_get_live_rssi(void)        { return -72.0f; }
uint16_t hw_subghz_capture_sample_count(void) { return s_last_raw.count; }
const SimSubghzDecoded *hw_subghz_capture_decoded(void) { return &s_last_decoded; }

/* -------- Replay / Encode TX -------- */

struct replay_ctx {
    hw_subghz_replay_cb_t cb;
    void                 *ctx;
    esp_timer_handle_t    timer;
};

static void replay_fire(void *arg)
{
    struct replay_ctx *r = (struct replay_ctx *)arg;
    if (r->cb) r->cb(true, r->ctx);
    esp_timer_delete(r->timer);
    free(r);
}

static void kick_replay(hw_subghz_replay_cb_t cb, void *ctx, uint32_t ms)
{
    struct replay_ctx *r = calloc(1, sizeof(*r));
    if (!r) return;
    r->cb = cb; r->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = replay_fire, .arg = r, .name = "replay_stub",
    };
    esp_timer_create(&args, &r->timer);
    esp_timer_start_once(r->timer, (uint64_t)ms * 1000);
}

void hw_subghz_start_replay(const SimSubghzRaw *raw, hw_subghz_replay_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_replay (%u samples)", raw ? raw->count : 0);
    kick_replay(cb, ctx, DEFER_MS_MED);
}

void hw_subghz_stop_replay(void)
{
    ESP_LOGW(TAG, "STUB: stop_replay");
}

void hw_subghz_start_encode_tx(const char *protocol, uint64_t data,
                               uint8_t bits, uint32_t te,
                               uint8_t repeat_count,
                               hw_subghz_replay_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_encode_tx proto=%s data=0x%llx bits=%u te=%u reps=%u",
             protocol ? protocol : "<null>", (unsigned long long)data,
             bits, (unsigned)te, repeat_count);
    kick_replay(cb, ctx, DEFER_MS_MED);
}

/* -------- Decode -------- */

struct decode_ctx {
    hw_subghz_decode_cb_t cb;
    void                 *ctx;
    esp_timer_handle_t    timer;
};

static struct decode_ctx *s_dec;

static void decode_fire(void *arg)
{
    struct decode_ctx *d = (struct decode_ctx *)arg;
    SimSubghzDecoded dec = {
        .protocol = "Princeton",
        .data     = 0xDEADBEULL,
        .bits     = 24,
        .te       = 400,
    };
    if (d->cb) d->cb(&dec, d->ctx);
}

void hw_subghz_start_decode(hw_subghz_decode_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_decode");
    hw_subghz_stop_decode();
    s_dec = calloc(1, sizeof(*s_dec));
    if (!s_dec) return;
    s_dec->cb = cb; s_dec->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = decode_fire, .arg = s_dec, .name = "dec_stub",
    };
    esp_timer_create(&args, &s_dec->timer);
    esp_timer_start_periodic(s_dec->timer, 2000 * 1000);
}

void hw_subghz_stop_decode(void)
{
    ESP_LOGW(TAG, "STUB: stop_decode");
    if (!s_dec) return;
    if (s_dec->timer) {
        esp_timer_stop(s_dec->timer);
        esp_timer_delete(s_dec->timer);
    }
    free(s_dec);
    s_dec = NULL;
}

/* -------- Read (history) -------- */

struct read_ctx {
    hw_subghz_read_cb_t cb;
    void               *ctx;
    esp_timer_handle_t  timer;
    uint16_t            idx;
};

static struct read_ctx *s_read;

static void read_fire(void *arg)
{
    struct read_ctx *r = (struct read_ctx *)arg;
    if (s_history_count < SUBGHZ_HISTORY_MAX) {
        SubghzHistoryEntry *e = &s_history[s_history_count];
        strcpy(e->protocol, "Princeton");
        e->data = 0xA5A50 + s_history_count;
        e->bits = 24;
        e->te = 400;
        e->freq_khz = s_frequency_khz;
        e->timestamp_ms = esp_timer_get_time() / 1000;
        e->proto_type = 1;
        s_history_count++;
    }
    if (r->cb) r->cb(&s_history[s_history_count - 1], s_history_count, r->ctx);
}

static void kick_read(hw_subghz_read_cb_t cb, void *ctx, bool hopping)
{
    hw_subghz_stop_read();
    s_read = calloc(1, sizeof(*s_read));
    if (!s_read) return;
    s_read->cb = cb; s_read->ctx = ctx;
    s_hopping = hopping;
    s_hop_locked = false;
    const esp_timer_create_args_t args = {
        .callback = read_fire, .arg = s_read, .name = "read_stub",
    };
    esp_timer_create(&args, &s_read->timer);
    esp_timer_start_periodic(s_read->timer, 1500 * 1000);
}

void hw_subghz_start_read(hw_subghz_read_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_read");
    kick_read(cb, ctx, false);
}

void hw_subghz_start_read_hopping(hw_subghz_read_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: start_read_hopping");
    kick_read(cb, ctx, true);
}

void hw_subghz_stop_read(void)
{
    ESP_LOGW(TAG, "STUB: stop_read");
    if (!s_read) return;
    if (s_read->timer) {
        esp_timer_stop(s_read->timer);
        esp_timer_delete(s_read->timer);
    }
    free(s_read);
    s_read = NULL;
    s_hopping = false;
}

bool     hw_subghz_is_hopping(void)      { return s_hopping; }
uint32_t hw_subghz_hop_current_freq(void){ return s_hop_current_khz; }
bool     hw_subghz_hop_is_locked(void)   { return s_hop_locked; }

/* -------- Ping / Relay -------- */

struct ping_ctx {
    hw_subghz_ping_cb_t cb;
    void               *ctx;
    esp_timer_handle_t  timer;
};

static void ping_fire(void *arg)
{
    struct ping_ctx *p = (struct ping_ctx *)arg;
    if (p->cb) p->cb(true, p->ctx);
    esp_timer_delete(p->timer);
    free(p);
}

void hw_subghz_ping(hw_subghz_ping_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: ping");
    struct ping_ctx *p = calloc(1, sizeof(*p));
    if (!p) return;
    p->cb = cb; p->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = ping_fire, .arg = p, .name = "ping_stub",
    };
    esp_timer_create(&args, &p->timer);
    esp_timer_start_once(p->timer, (uint64_t)DEFER_MS_SHORT * 1000);
}

struct relay_ctx {
    hw_subghz_relay_cb_t cb;
    void                *ctx;
    esp_timer_handle_t   timer;
    int                  progress;
};

static void relay_fire(void *arg)
{
    struct relay_ctx *r = (struct relay_ctx *)arg;
    r->progress += 25;
    if (r->cb) r->cb(r->progress >= 100, r->progress, r->ctx);
    if (r->progress >= 100) {
        esp_timer_stop(r->timer);
        esp_timer_delete(r->timer);
        free(r);
    }
}

static void kick_relay(hw_subghz_relay_cb_t cb, void *ctx)
{
    struct relay_ctx *r = calloc(1, sizeof(*r));
    if (!r) return;
    r->cb = cb; r->ctx = ctx; r->progress = 0;
    const esp_timer_create_args_t args = {
        .callback = relay_fire, .arg = r, .name = "relay_stub",
    };
    esp_timer_create(&args, &r->timer);
    esp_timer_start_periodic(r->timer, 300 * 1000);
}

void hw_subghz_send_dump(hw_subghz_relay_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: send_dump"); kick_relay(cb, ctx);
}

void hw_subghz_recv_dump(hw_subghz_relay_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: recv_dump"); kick_relay(cb, ctx);
}

void hw_subghz_stop_relay(void)
{
    ESP_LOGW(TAG, "STUB: stop_relay");
}

/* -------- Config -------- */

uint32_t hw_subghz_get_frequency(void)         { return s_frequency_khz; }
void     hw_subghz_set_frequency(uint32_t khz) { s_frequency_khz = khz; }
uint8_t  hw_subghz_get_preset(void)            { return s_preset; }
void     hw_subghz_set_preset(uint8_t p)       { s_preset = p; }

struct mfc_dump *hw_subghz_get_relay_dump(void) { return &s_relay_dump; }

const SubghzHistoryEntry *hw_subghz_get_read_history(uint16_t *count)
{
    if (count) *count = s_history_count;
    return s_history;
}
