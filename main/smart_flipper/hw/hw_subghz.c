/* Stub SubGHz backend: canned data so analyzer/capture/replay/decode/ping/relay
 * exercise without the radio.
 *
 * Async-slot lifecycle invariant (applied to every s_* tracker below):
 *   one-shot fire: detach s_X -> snapshot locals -> free struct -> user cb ->
 *                  delete timer last (esp_timer needs the handle valid until
 *                  the cb returns).
 *   periodic fire: barrier check (s_X == self), then invoke user cb. Skips the
 *                  one extra firing esp_timer may dispatch after stop_X.
 *   stop:          atomically claim s_X; if non-NULL, stop+delete timer, free.
 * Tracker reads/writes use __atomic_* so the LVGL task and the esp_timer task
 * don't tear across the dual-core boundary. */

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

#define LOAD(p)        __atomic_load_n(&(p), __ATOMIC_ACQUIRE)
#define CLAIM(p)       __atomic_exchange_n(&(p), NULL, __ATOMIC_ACQ_REL)
#define DETACH(p, me)  do { __typeof__(p) _e = (me); \
    __atomic_compare_exchange_n(&(p), &_e, NULL, false, \
                                __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); } while(0)

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
    if (LOAD(s_az) != a) return;
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
    struct analyzer_ctx *a = calloc(1, sizeof(*a));
    if (!a) return;
    a->cb = cb; a->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = analyzer_tick, .arg = a, .name = "az_stub",
    };
    if (esp_timer_create(&args, &a->timer) != ESP_OK) { free(a); return; }
    __atomic_store_n(&s_az, a, __ATOMIC_RELEASE);
    esp_timer_start_periodic(a->timer, 100 * 1000);
}

void hw_subghz_stop_analyzer(void)
{
    ESP_LOGW(TAG, "STUB: stop_analyzer");
    struct analyzer_ctx *a = CLAIM(s_az);
    if (!a) return;
    esp_timer_stop(a->timer);
    esp_timer_delete(a->timer);
    free(a);
}

struct capture_ctx {
    hw_subghz_capture_cb_t cb;
    void                  *ctx;
    esp_timer_handle_t     timer;
};

static struct capture_ctx *s_cap;

static void capture_fire(void *arg)
{
    struct capture_ctx *c = (struct capture_ctx *)arg;
    DETACH(s_cap, c);

    s_capturing = false;
    s_last_raw.count = 16;
    s_last_raw.full_count = 16;
    for (int i = 0; i < 16; i++) {
        s_last_raw.samples[i] = (i & 1) ? 450 : -450;
    }
    s_last_decoded.protocol = "Princeton";
    s_last_decoded.data = 0xA5A5A5ULL;
    s_last_decoded.bits = 24;
    s_last_decoded.te   = 400;

    hw_subghz_capture_cb_t cb = c->cb;
    void                  *ctx = c->ctx;
    esp_timer_handle_t     t   = c->timer;
    free(c);
    if (cb) cb(true, &s_last_raw, ctx);
    esp_timer_delete(t);
}

static void kick_capture_defer(hw_subghz_capture_cb_t cb, void *ctx, uint32_t ms)
{
    hw_subghz_stop_capture();
    struct capture_ctx *c = calloc(1, sizeof(*c));
    if (!c) return;
    c->cb = cb; c->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = capture_fire, .arg = c, .name = "cap_stub",
    };
    if (esp_timer_create(&args, &c->timer) != ESP_OK) { free(c); return; }
    s_capturing = true;
    __atomic_store_n(&s_cap, c, __ATOMIC_RELEASE);
    esp_timer_start_once(c->timer, (uint64_t)ms * 1000);
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
    struct capture_ctx *c = CLAIM(s_cap);
    s_capturing = false;
    if (!c) return;
    esp_timer_stop(c->timer);
    esp_timer_delete(c->timer);
    free(c);
}

bool hw_subghz_is_capturing(void)          { return s_capturing; }
float hw_subghz_get_live_rssi(void)        { return -72.0f; }
uint16_t hw_subghz_capture_sample_count(void) { return s_last_raw.count; }
const SimSubghzDecoded *hw_subghz_capture_decoded(void) { return &s_last_decoded; }

struct replay_ctx {
    hw_subghz_replay_cb_t cb;
    void                 *ctx;
    esp_timer_handle_t    timer;
};

static struct replay_ctx *s_replay;

static void replay_fire(void *arg)
{
    struct replay_ctx *r = (struct replay_ctx *)arg;
    DETACH(s_replay, r);

    hw_subghz_replay_cb_t cb = r->cb;
    void                 *ctx = r->ctx;
    esp_timer_handle_t    t   = r->timer;
    free(r);
    if (cb) cb(true, ctx);
    esp_timer_delete(t);
}

static void kick_replay(hw_subghz_replay_cb_t cb, void *ctx, uint32_t ms)
{
    hw_subghz_stop_replay();
    struct replay_ctx *r = calloc(1, sizeof(*r));
    if (!r) return;
    r->cb = cb; r->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = replay_fire, .arg = r, .name = "replay_stub",
    };
    if (esp_timer_create(&args, &r->timer) != ESP_OK) { free(r); return; }
    __atomic_store_n(&s_replay, r, __ATOMIC_RELEASE);
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
    struct replay_ctx *r = CLAIM(s_replay);
    if (!r) return;
    esp_timer_stop(r->timer);
    esp_timer_delete(r->timer);
    free(r);
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

struct decode_ctx {
    hw_subghz_decode_cb_t cb;
    void                 *ctx;
    esp_timer_handle_t    timer;
};

static struct decode_ctx *s_dec;

static void decode_fire(void *arg)
{
    struct decode_ctx *d = (struct decode_ctx *)arg;
    if (LOAD(s_dec) != d) return;
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
    struct decode_ctx *d = calloc(1, sizeof(*d));
    if (!d) return;
    d->cb = cb; d->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = decode_fire, .arg = d, .name = "dec_stub",
    };
    if (esp_timer_create(&args, &d->timer) != ESP_OK) { free(d); return; }
    __atomic_store_n(&s_dec, d, __ATOMIC_RELEASE);
    esp_timer_start_periodic(d->timer, 2000 * 1000);
}

void hw_subghz_stop_decode(void)
{
    ESP_LOGW(TAG, "STUB: stop_decode");
    struct decode_ctx *d = CLAIM(s_dec);
    if (!d) return;
    esp_timer_stop(d->timer);
    esp_timer_delete(d->timer);
    free(d);
}

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
    if (LOAD(s_read) != r) return;
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
    if (s_history_count == 0) return;
    if (r->cb) r->cb(&s_history[s_history_count - 1], s_history_count, r->ctx);
}

static void kick_read(hw_subghz_read_cb_t cb, void *ctx, bool hopping)
{
    hw_subghz_stop_read();
    struct read_ctx *r = calloc(1, sizeof(*r));
    if (!r) return;
    r->cb = cb; r->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = read_fire, .arg = r, .name = "read_stub",
    };
    if (esp_timer_create(&args, &r->timer) != ESP_OK) { free(r); return; }
    s_hopping = hopping;
    s_hop_locked = false;
    __atomic_store_n(&s_read, r, __ATOMIC_RELEASE);
    esp_timer_start_periodic(r->timer, 1500 * 1000);
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
    struct read_ctx *r = CLAIM(s_read);
    s_hopping = false;
    if (!r) return;
    esp_timer_stop(r->timer);
    esp_timer_delete(r->timer);
    free(r);
}

bool     hw_subghz_is_hopping(void)      { return s_hopping; }
uint32_t hw_subghz_hop_current_freq(void){ return s_hop_current_khz; }
bool     hw_subghz_hop_is_locked(void)   { return s_hop_locked; }

struct ping_ctx {
    hw_subghz_ping_cb_t cb;
    void               *ctx;
    esp_timer_handle_t  timer;
};

static struct ping_ctx *s_ping;

static void ping_fire(void *arg)
{
    struct ping_ctx *p = (struct ping_ctx *)arg;
    DETACH(s_ping, p);

    hw_subghz_ping_cb_t cb = p->cb;
    void               *ctx = p->ctx;
    esp_timer_handle_t  t   = p->timer;
    free(p);
    if (cb) cb(true, ctx);
    esp_timer_delete(t);
}

void hw_subghz_ping(hw_subghz_ping_cb_t cb, void *ctx)
{
    ESP_LOGW(TAG, "STUB: ping");
    struct ping_ctx *prev = CLAIM(s_ping);
    if (prev) {
        esp_timer_stop(prev->timer);
        esp_timer_delete(prev->timer);
        free(prev);
    }
    struct ping_ctx *p = calloc(1, sizeof(*p));
    if (!p) return;
    p->cb = cb; p->ctx = ctx;
    const esp_timer_create_args_t args = {
        .callback = ping_fire, .arg = p, .name = "ping_stub",
    };
    if (esp_timer_create(&args, &p->timer) != ESP_OK) { free(p); return; }
    __atomic_store_n(&s_ping, p, __ATOMIC_RELEASE);
    esp_timer_start_once(p->timer, (uint64_t)DEFER_MS_SHORT * 1000);
}

struct relay_ctx {
    hw_subghz_relay_cb_t cb;
    void                *ctx;
    esp_timer_handle_t   timer;
    int                  progress;
};

static struct relay_ctx *s_relay;

static void relay_fire(void *arg)
{
    struct relay_ctx *r = (struct relay_ctx *)arg;
    if (LOAD(s_relay) != r) return;

    r->progress += 25;
    bool done = r->progress >= 100;
    int  prog = r->progress;
    hw_subghz_relay_cb_t cb = r->cb;
    void                *ctx = r->ctx;

    if (done) {
        DETACH(s_relay, r);
        esp_timer_handle_t t = r->timer;
        free(r);
        if (cb) cb(true, prog, ctx);
        esp_timer_stop(t);
        esp_timer_delete(t);
    } else {
        if (cb) cb(false, prog, ctx);
    }
}

static void kick_relay(hw_subghz_relay_cb_t cb, void *ctx)
{
    hw_subghz_stop_relay();
    struct relay_ctx *r = calloc(1, sizeof(*r));
    if (!r) return;
    r->cb = cb; r->ctx = ctx; r->progress = 0;
    const esp_timer_create_args_t args = {
        .callback = relay_fire, .arg = r, .name = "relay_stub",
    };
    if (esp_timer_create(&args, &r->timer) != ESP_OK) { free(r); return; }
    __atomic_store_n(&s_relay, r, __ATOMIC_RELEASE);
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
    struct relay_ctx *r = CLAIM(s_relay);
    if (!r) return;
    esp_timer_stop(r->timer);
    esp_timer_delete(r->timer);
    free(r);
}

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
