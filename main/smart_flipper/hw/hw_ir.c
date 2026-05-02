#include "hw_ir.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "hw_ir";

#define IR_TX_GPIO          16
#define IR_RX_GPIO          17
#define IR_RMT_RESOLUTION   1000000U   /* 1 us/tick so timings are direct us */
#define IR_TX_MEM_SYMBOLS   64
#define IR_RX_MEM_SYMBOLS   128
#define IR_TX_QUEUE_DEPTH   4
/* 0.33 matches Flipper + IR industry; 0.50 risks TV AGC mismatch. */
#define IR_CARRIER_DUTY     0.33f

#define IR_MAX_TIMINGS      1024       /* NEC ~67, AC frames ~600 */
#define IR_RX_MAX_SYMBOLS   512

/* End-of-frame: longest AC inter-frame gap is ~20 ms. */
#define IR_RX_TIMEOUT_NS    50000000ULL
/* IDF v6.1 caps signal_range_min at the RMT source clock period (~3200 ns
 * on this chip), so this is not a real noise floor; TSOP14438 rejects
 * sub-100us edges upstream. */
#define IR_RX_MIN_NS        1000ULL
#define IR_RX_MAX_NS        30000000ULL

static rmt_channel_handle_t  s_tx_chan;
static rmt_encoder_handle_t  s_copy_encoder;
static uint32_t              s_carrier_hz_active;
static bool                  s_inited;
static bool                  s_tx_invert_active;
static int64_t               s_last_tx_done_us;
static SemaphoreHandle_t     s_tx_mtx;
static volatile bool         s_log_next_send;

void hw_ir_log_next_send(void) { s_log_next_send = true; }

#define HW_IR_TX_QUEUE_DEPTH    8
#define HW_IR_TX_WORKER_STACK   4096
#define HW_IR_TX_WORKER_PRIO    (tskIDLE_PRIORITY + 4)
#define HW_IR_TX_WORKER_CORE    1
#define HW_IR_TX_MAX_REPEAT     32

typedef struct {
    uint32_t            id;
    uint16_t           *timings;       /* heap-owned */
    size_t              n_timings;
    uint32_t            carrier_hz;
    uint8_t             repeat;
    uint16_t            gap_ms;
    hw_ir_tx_done_cb_t  on_done;
    void               *ctx;
} tx_cmd_t;

static QueueHandle_t   s_tx_cmd_queue;
static SemaphoreHandle_t s_tx_idle_sem;
static TaskHandle_t    s_tx_worker;
static volatile uint32_t s_tx_next_id = 1;
static volatile uint32_t s_tx_cancel_id;
static volatile bool   s_tx_cancel_all;
static volatile uint32_t s_tx_current_id;
static portMUX_TYPE    s_tx_id_lock = portMUX_INITIALIZER_UNLOCKED;

static void tx_cmd_free(tx_cmd_t *c)
{
    if(c && c->timings) free(c->timings);
}

static void tx_cmd_finish(tx_cmd_t *c, HwIrTxResult result)
{
    if(c->on_done) c->on_done(c->id, result, c->ctx);
    tx_cmd_free(c);
}

/* MUST NOT clear s_tx_cancel_id -- cancel must remain armed for the whole
 * burst, otherwise a re-check after the gap-poll would let the worker
 * keep firing past the cancel point. */
static bool worker_should_cancel(uint32_t id)
{
    if(s_tx_cancel_all) return true;
    if(s_tx_cancel_id && s_tx_cancel_id == id) return true;
    return false;
}

static void worker_consume_cancel_id(uint32_t id)
{
    if(s_tx_cancel_id != id) return;
    portENTER_CRITICAL(&s_tx_id_lock);
    if(s_tx_cancel_id == id) s_tx_cancel_id = 0;
    portEXIT_CRITICAL(&s_tx_id_lock);
}

static void tx_worker_fn(void *arg)
{
    (void)arg;
    for(;;) {
        tx_cmd_t cmd;
        if(xQueueReceive(s_tx_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) continue;

        portENTER_CRITICAL(&s_tx_id_lock);
        s_tx_current_id = cmd.id;
        portEXIT_CRITICAL(&s_tx_id_lock);

        HwIrTxResult result = HW_IR_TX_DONE;

        if(worker_should_cancel(cmd.id)) {
            result = HW_IR_TX_CANCELLED;
        } else {
            uint8_t reps = cmd.repeat ? cmd.repeat : 1;
            if(reps > HW_IR_TX_MAX_REPEAT) reps = HW_IR_TX_MAX_REPEAT;
            for(uint8_t i = 0; i < reps; i++) {
                if(worker_should_cancel(cmd.id)) { result = HW_IR_TX_CANCELLED; break; }
                esp_err_t err = hw_ir_send_raw(cmd.timings, cmd.n_timings, cmd.carrier_hz);
                if(err != ESP_OK) { result = HW_IR_TX_ERROR; break; }
                if(i + 1 < reps && cmd.gap_ms) {
                    /* Poll cancel during gap so cancellation is bounded. */
                    uint32_t left = cmd.gap_ms;
                    while(left > 0 && !worker_should_cancel(cmd.id)) {
                        uint32_t chunk = left > 20 ? 20 : left;
                        vTaskDelay(pdMS_TO_TICKS(chunk));
                        left -= chunk;
                    }
                    if(worker_should_cancel(cmd.id)) { result = HW_IR_TX_CANCELLED; break; }
                }
            }
        }

        portENTER_CRITICAL(&s_tx_id_lock);
        s_tx_current_id = 0;
        portEXIT_CRITICAL(&s_tx_id_lock);

        /* Consume matching cancel id so a re-used id isn't auto-killed. */
        worker_consume_cancel_id(cmd.id);

        tx_cmd_finish(&cmd, result);

        if(uxQueueMessagesWaiting(s_tx_cmd_queue) == 0) {
            s_tx_cancel_all = false;
            if(s_tx_idle_sem) xSemaphoreGive(s_tx_idle_sem);
        }
    }
}

static atomic_int s_tx_engine_state;

static void tx_engine_start_once(void)
{
    int expected = 0;
    if(atomic_compare_exchange_strong(&s_tx_engine_state, &expected, 1)) {
        s_tx_cmd_queue = xQueueCreate(HW_IR_TX_QUEUE_DEPTH, sizeof(tx_cmd_t));
        s_tx_idle_sem  = xSemaphoreCreateBinary();
        assert(s_tx_cmd_queue && s_tx_idle_sem);
        BaseType_t ok = xTaskCreatePinnedToCore(tx_worker_fn, "hw_ir_tx",
                                                HW_IR_TX_WORKER_STACK, NULL,
                                                HW_IR_TX_WORKER_PRIO, &s_tx_worker,
                                                HW_IR_TX_WORKER_CORE);
        assert(ok == pdPASS);
        atomic_store(&s_tx_engine_state, 2);
        return;
    }
    while(atomic_load(&s_tx_engine_state) != 2) {
        vTaskDelay(1);
    }
}

uint32_t hw_ir_tx_submit(const HwIrTxRequest *req)
{
    if(!s_inited || !req)                        return 0;
    if(!req->timings || req->n_timings == 0)     return 0;
    if(req->n_timings > IR_MAX_TIMINGS)           return 0;

    tx_engine_start_once();

    tx_cmd_t cmd = {0};
    cmd.timings = malloc(req->n_timings * sizeof(uint16_t));
    if(!cmd.timings) return 0;
    memcpy(cmd.timings, req->timings, req->n_timings * sizeof(uint16_t));
    cmd.n_timings  = req->n_timings;
    cmd.carrier_hz = req->carrier_hz ? req->carrier_hz : 38000;
    cmd.repeat     = req->repeat ? req->repeat : 1;
    cmd.gap_ms     = req->gap_ms;
    cmd.on_done    = req->on_done;
    cmd.ctx        = req->ctx;

    portENTER_CRITICAL(&s_tx_id_lock);
    cmd.id = s_tx_next_id++;
    if(s_tx_next_id == 0) s_tx_next_id = 1;
    portEXIT_CRITICAL(&s_tx_id_lock);

    /* Drop stale idle ack before queuing. */
    xSemaphoreTake(s_tx_idle_sem, 0);

    if(xQueueSend(s_tx_cmd_queue, &cmd, 0) != pdTRUE) {
        free(cmd.timings);
        return 0;
    }
    return cmd.id;
}

void hw_ir_tx_cancel(uint32_t id)
{
    if(id == 0) return;
    portENTER_CRITICAL(&s_tx_id_lock);
    s_tx_cancel_id = id;
    portEXIT_CRITICAL(&s_tx_id_lock);
}

void hw_ir_tx_cancel_all(uint32_t timeout_ms)
{
    if(!s_tx_cmd_queue) return;
    s_tx_cancel_all = true;

    /* Fire on_done(CANCELLED) for queued-but-not-yet-running cmds so
     * callers can balance their state. */
    tx_cmd_t cmd;
    while(xQueueReceive(s_tx_cmd_queue, &cmd, 0) == pdTRUE) {
        tx_cmd_finish(&cmd, HW_IR_TX_CANCELLED);
    }

    if(s_tx_idle_sem) {
        if(s_tx_current_id != 0) {
            xSemaphoreTake(s_tx_idle_sem,
                           timeout_ms ? pdMS_TO_TICKS(timeout_ms) : 0);
        }
    }
    s_tx_cancel_all = false;
}

static rmt_channel_handle_t  s_rx_chan;
static rmt_symbol_word_t    *s_rx_buf;
static QueueHandle_t         s_rx_evt_queue;
static TaskHandle_t          s_rx_task;
static hw_ir_rx_cb_t         s_rx_user_cb;
static void                 *s_rx_user_ctx;
static hw_ir_rx_edge_cb_t    s_rx_edge_cb;
static void                 *s_rx_edge_ctx;
static bool                  s_rx_running;
static bool                  s_rx_chan_enabled;
static SemaphoreHandle_t     s_rx_done_sem;

typedef struct {
    rmt_symbol_word_t *symbols;
    size_t             count;
} rx_evt_t;

static esp_err_t apply_carrier(uint32_t carrier_hz)
{
    /* Apply fresh every TX -- IDF v6.1 carrier regs drift after pause/resume.
     * carrier_hz==0 -> rmt_apply_carrier(NULL); rmt_apply_carrier asserts
     * high_ticks/low_ticks >= 1 so duty=0 isn't an option. */
    if(carrier_hz == 0) {
        esp_err_t err = rmt_apply_carrier(s_tx_chan, NULL);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "rmt_apply_carrier(off): %s", esp_err_to_name(err));
            return err;
        }
        s_carrier_hz_active = 0;
        return ESP_OK;
    }

    rmt_carrier_config_t cfg = {
        .frequency_hz     = carrier_hz,
        .duty_cycle       = IR_CARRIER_DUTY,
        .flags.polarity_active_low = false,
    };
    esp_err_t err = rmt_apply_carrier(s_tx_chan, &cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_apply_carrier(%u): %s", (unsigned)carrier_hz, esp_err_to_name(err));
        return err;
    }
    s_carrier_hz_active = carrier_hz;
    return ESP_OK;
}

static esp_err_t build_tx_channel(bool invert)
{
    (void)invert;
    const rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = IR_TX_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = IR_RMT_RESOLUTION,
        .mem_block_symbols = IR_TX_MEM_SYMBOLS,
        .trans_queue_depth = IR_TX_QUEUE_DEPTH,
        .flags.invert_out  = false,
        .flags.with_dma    = false,
    };
    esp_err_t err = rmt_new_tx_channel(&tx_cfg, &s_tx_chan);
    if(err != ESP_OK) return err;
    gpio_set_drive_capability(IR_TX_GPIO, GPIO_DRIVE_CAP_3);
    return ESP_OK;
}

esp_err_t hw_ir_set_invert(bool invert)
{
    (void)invert;
    return ESP_OK;
}

void hw_ir_init(void)
{
    if(s_inited) return;

    gpio_hold_dis(IR_TX_GPIO);
    ESP_ERROR_CHECK(build_tx_channel(s_tx_invert_active));

    /* IDF v6.1 made rmt_copy_encoder_config_t empty; {0} fires
     * -Werror=excess-initializers, so use the empty-aggregate form. */
    const rmt_copy_encoder_config_t enc_cfg = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&enc_cfg, &s_copy_encoder));

    /* Carrier OFF at init: IDF v6.x leaks the modulator to the pin between
     * rmt_enable and the first transmit, lighting the IR LED visibly on a
     * phone camera. apply_carrier() runs again per send. */
    ESP_ERROR_CHECK(apply_carrier(0));
    ESP_ERROR_CHECK(rmt_enable(s_tx_chan));

    s_tx_mtx = xSemaphoreCreateMutex();
    assert(s_tx_mtx);

    s_inited = true;
    ESP_LOGI(TAG, "init: TX=GPIO%d, %u Hz tick, carrier disabled until first send",
             IR_TX_GPIO, (unsigned)IR_RMT_RESOLUTION);
}

void hw_ir_quiesce_for_deep_sleep(void)
{
    if(s_tx_mtx) xSemaphoreTake(s_tx_mtx, pdMS_TO_TICKS(200));

    if(s_tx_chan) {
        apply_carrier(0);
        rmt_disable(s_tx_chan);
        rmt_del_channel(s_tx_chan);
        s_tx_chan = NULL;
    }
    if(s_copy_encoder) {
        rmt_del_encoder(s_copy_encoder);
        s_copy_encoder = NULL;
    }
    s_inited = false;

    gpio_config_t cfg = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << IR_TX_GPIO,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(IR_TX_GPIO, 0);
    gpio_hold_en(IR_TX_GPIO);

    if(s_tx_mtx) xSemaphoreGive(s_tx_mtx);
}

esp_err_t hw_ir_send_raw(const uint16_t *timings, size_t n_timings, uint32_t carrier_hz)
{
    if(!s_inited)                       return ESP_ERR_INVALID_STATE;
    if(timings == NULL || n_timings == 0) return ESP_ERR_INVALID_ARG;
    if(n_timings > IR_MAX_TIMINGS)      return ESP_ERR_INVALID_SIZE;

    if(s_tx_mtx) xSemaphoreTake(s_tx_mtx, portMAX_DELAY);

    const uint32_t req_hz = carrier_hz ? carrier_hz : 38000;
    esp_err_t err = apply_carrier(req_hz);
    if(s_log_next_send) {
        s_log_next_send = false;
        const size_t dump = n_timings < 8 ? n_timings : 8;
        ESP_LOGI(TAG,
                 "send n=%u req_hz=%lu active_hz=%lu apply=%s "
                 "t[0..%u]=%u %u %u %u %u %u %u %u",
                 (unsigned)n_timings, (unsigned long)req_hz,
                 (unsigned long)s_carrier_hz_active, esp_err_to_name(err),
                 (unsigned)dump,
                 dump > 0 ? timings[0] : 0, dump > 1 ? timings[1] : 0,
                 dump > 2 ? timings[2] : 0, dump > 3 ? timings[3] : 0,
                 dump > 4 ? timings[4] : 0, dump > 5 ? timings[5] : 0,
                 dump > 6 ? timings[6] : 0, dump > 7 ? timings[7] : 0);
    }
    if(err != ESP_OK) {
        if(s_tx_mtx) xSemaphoreGive(s_tx_mtx);
        return err;
    }

    /* Static symbol buffer (2 KB). s_tx_mtx serializes callers; the
     * stack version overflowed runner_task's 4 KB stack on heavy frames.
     * Odd-count buffers (final mark, no trailing space) are padded with a
     * 0-duration LOW phase so RMT terminates cleanly. */
    const size_t n_symbols = (n_timings + 1) / 2;
    static rmt_symbol_word_t symbols[(IR_MAX_TIMINGS + 1) / 2];

    for(size_t i = 0; i < n_symbols; i++) {
        const size_t i0 = i * 2;
        const size_t i1 = i0 + 1;
        uint16_t d0 = timings[i0];
        if(d0 > 32767) d0 = 32767;           /* RMT duration is 15 bits */
        symbols[i].level0    = 1;
        symbols[i].duration0 = d0;
        if(i1 < n_timings) {
            uint16_t d1 = timings[i1];
            if(d1 > 32767) d1 = 32767;
            symbols[i].level1    = 0;
            symbols[i].duration1 = d1;
        } else {
            symbols[i].level1    = 0;
            symbols[i].duration1 = 0;
        }
    }

    const rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
        .flags.eot_level = 0,
    };

    err = rmt_transmit(s_tx_chan, s_copy_encoder, symbols,
                       n_symbols * sizeof(symbols[0]), &tx_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_transmit: %s", esp_err_to_name(err));
        if(s_tx_mtx) xSemaphoreGive(s_tx_mtx);
        return err;
    }
    err = rmt_tx_wait_all_done(s_tx_chan, pdMS_TO_TICKS(500));
    s_last_tx_done_us = esp_timer_get_time();
    if(s_tx_mtx) xSemaphoreGive(s_tx_mtx);
    return err;
}

bool hw_ir_tx_was_recent_us(int64_t window_us)
{
    if(s_last_tx_done_us == 0) return false;
    return (esp_timer_get_time() - s_last_tx_done_us) < window_us;
}

typedef struct {
    uint16_t *timings;
    size_t    n;
    uint16_t *repeat;        /* NULL -> worker re-fires `timings` */
    size_t    repeat_n;
    uint32_t  freq_hz;
    uint32_t  period_ms;
    uint32_t  min_repeats;   /* total frame floor incl. initial */
    uint32_t  sent;          /* counts initial + worker repeats */
} repeat_args_t;

static TaskHandle_t       s_repeat_task;
static SemaphoreHandle_t  s_repeat_done_sem;
static volatile bool      s_repeat_running;

static void repeat_task_fn(void *arg)
{
    repeat_args_t *a = arg;
    const uint16_t *hold_t = a->repeat   ? a->repeat   : a->timings;
    size_t          hold_n = a->repeat_n ? a->repeat_n : a->n;
    while(s_repeat_running || a->sent < a->min_repeats) {
        vTaskDelay(pdMS_TO_TICKS(a->period_ms));
        if(!s_repeat_running && a->sent >= a->min_repeats) break;
        hw_ir_send_raw(hold_t, hold_n, a->freq_hz);
        a->sent++;
    }
    free(a->timings);
    if(a->repeat) free(a->repeat);
    free(a);
    s_repeat_task = NULL;
    if(s_repeat_done_sem) xSemaphoreGive(s_repeat_done_sem);
    vTaskDelete(NULL);
}

esp_err_t hw_ir_send_repeat_start(const uint16_t *timings, size_t n_timings,
                                  const uint16_t *repeat_timings, size_t repeat_n_timings,
                                  uint32_t carrier_hz, uint32_t period_ms,
                                  uint32_t min_repeats)
{
    if(!s_inited)                         return ESP_ERR_INVALID_STATE;
    if(timings == NULL || n_timings == 0) return ESP_ERR_INVALID_ARG;

    if(!s_repeat_done_sem) s_repeat_done_sem = xSemaphoreCreateBinary();

    /* Defensive drain: prior _stop may have timed out. */
    if(s_repeat_task != NULL || s_repeat_running) {
        s_repeat_running = false;
        xSemaphoreTake(s_repeat_done_sem, pdMS_TO_TICKS(2000));
    }
    xSemaphoreTake(s_repeat_done_sem, 0);

    repeat_args_t *a = calloc(1, sizeof(repeat_args_t));
    if(!a) return ESP_ERR_NO_MEM;
    a->timings = malloc(n_timings * sizeof(uint16_t));
    if(!a->timings) { free(a); return ESP_ERR_NO_MEM; }
    memcpy(a->timings, timings, n_timings * sizeof(uint16_t));
    a->n         = n_timings;
    if(repeat_timings && repeat_n_timings) {
        a->repeat = malloc(repeat_n_timings * sizeof(uint16_t));
        if(!a->repeat) { free(a->timings); free(a); return ESP_ERR_NO_MEM; }
        memcpy(a->repeat, repeat_timings, repeat_n_timings * sizeof(uint16_t));
        a->repeat_n = repeat_n_timings;
    }
    a->freq_hz     = carrier_hz ? carrier_hz : 38000;
    a->period_ms   = period_ms ? period_ms : 110;
    a->min_repeats = min_repeats;
    a->sent        = 0;

    esp_err_t err = hw_ir_send_raw(a->timings, n_timings, a->freq_hz);
    if(err != ESP_OK) {
        free(a->timings);
        if(a->repeat) free(a->repeat);
        free(a);
        return err;
    }
    a->sent = 1;

    s_repeat_running = true;
    if(xTaskCreatePinnedToCore(repeat_task_fn, "hw_ir_rep", 3072, a,
                               tskIDLE_PRIORITY + 3, &s_repeat_task, 1) != pdPASS) {
        s_repeat_running = false;
        free(a->timings);
        if(a->repeat) free(a->repeat);
        free(a);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void hw_ir_send_repeat_stop(void)
{
    if(!s_repeat_running && s_repeat_task == NULL) return;
    s_repeat_running = false;
    if(s_repeat_done_sem) {
        xSemaphoreTake(s_repeat_done_sem, pdMS_TO_TICKS(2000));
    }
}

static IRAM_ATTR bool rx_done_isr(rmt_channel_handle_t channel,
                                  const rmt_rx_done_event_data_t *edata,
                                  void *user_ctx)
{
    (void)channel; (void)user_ctx;
    BaseType_t hp_woken = pdFALSE;
    rx_evt_t evt = {
        .symbols = edata->received_symbols,
        .count   = edata->num_symbols,
    };
    xQueueSendFromISR(s_rx_evt_queue, &evt, &hp_woken);
    return hp_woken == pdTRUE;
}

static void rx_task(void *arg)
{
    (void)arg;

    static const rmt_receive_config_t rcv_cfg = {
        .signal_range_min_ns = IR_RX_MIN_NS,
        .signal_range_max_ns = IR_RX_MAX_NS,
    };

    static uint16_t timings[IR_RX_MAX_SYMBOLS * 2];

    esp_err_t err = rmt_receive(s_rx_chan, s_rx_buf,
                                IR_RX_MAX_SYMBOLS * sizeof(rmt_symbol_word_t),
                                &rcv_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_receive prime: %s", esp_err_to_name(err));
        s_rx_task = NULL;
        if(s_rx_done_sem) xSemaphoreGive(s_rx_done_sem);
        vTaskDelete(NULL);
    }

    while(s_rx_running) {
        rx_evt_t evt;
        if(xQueueReceive(s_rx_evt_queue, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if(!s_rx_running) break;

        if(s_rx_edge_cb) {
            uint32_t edges = 0;
            for(size_t i = 0; i < evt.count; i++) {
                const rmt_symbol_word_t s = evt.symbols[i];
                if(s.duration0 == 0) break;
                /* TSOP14438 idles HIGH -> level==0 means IR mark; invert
                 * here so callers always see mark-first semantics. */
                s_rx_edge_cb(s.level0 == 0, s.duration0, false, s_rx_edge_ctx);
                edges++;
                if(s.duration1 == 0) break;
                s_rx_edge_cb(s.level1 == 0, s.duration1, false, s_rx_edge_ctx);
                edges++;
            }
            if(edges > 0) {
                s_rx_edge_cb(false, 0, true, s_rx_edge_ctx);
            }
        } else if(s_rx_user_cb) {
            size_t out_n = 0;
            bool started = false;
            for(size_t i = 0; i < evt.count; i++) {
                const rmt_symbol_word_t s = evt.symbols[i];

                if(s.duration0 == 0) break;
                const bool ph0_is_mark = (s.level0 == 0);
                if(!started) {
                    if(ph0_is_mark) { timings[out_n++] = s.duration0; started = true; }
                } else if(out_n + 1 < sizeof(timings)/sizeof(timings[0])) {
                    timings[out_n++] = s.duration0;
                } else break;

                if(s.duration1 == 0) break;
                if(!started) {
                    const bool ph1_is_mark = (s.level1 == 0);
                    if(ph1_is_mark) { timings[out_n++] = s.duration1; started = true; }
                } else if(out_n + 1 < sizeof(timings)/sizeof(timings[0])) {
                    timings[out_n++] = s.duration1;
                } else break;
            }
            if(out_n >= 2) s_rx_user_cb(timings, out_n, s_rx_user_ctx);
        }

        if(s_rx_running) {
            err = rmt_receive(s_rx_chan, s_rx_buf,
                              IR_RX_MAX_SYMBOLS * sizeof(rmt_symbol_word_t),
                              &rcv_cfg);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "rmt_receive re-arm: %s", esp_err_to_name(err));
                break;
            }
        }
    }

    s_rx_task = NULL;
    if(s_rx_done_sem) xSemaphoreGive(s_rx_done_sem);
    vTaskDelete(NULL);
}

static void rx_channel_init_once(void)
{
    if(s_rx_chan != NULL) return;
    const rmt_rx_channel_config_t rx_cfg = {
        .gpio_num          = IR_RX_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = IR_RMT_RESOLUTION,
        .mem_block_symbols = IR_RX_MEM_SYMBOLS,
        .flags.invert_in   = false,
        .flags.with_dma    = false,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &s_rx_chan));

    s_rx_buf = malloc(IR_RX_MAX_SYMBOLS * sizeof(rmt_symbol_word_t));
    assert(s_rx_buf);

    const rmt_rx_event_callbacks_t cbs = { .on_recv_done = rx_done_isr };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(s_rx_chan, &cbs, NULL));

    s_rx_evt_queue = xQueueCreate(8, sizeof(rx_evt_t));
    assert(s_rx_evt_queue);

    s_rx_done_sem = xSemaphoreCreateBinary();
    assert(s_rx_done_sem);
}

static void rx_start_common(void)
{
    rx_channel_init_once();
    if(!s_rx_chan_enabled) {
        esp_err_t err = rmt_enable(s_rx_chan);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "rmt_enable: %s", esp_err_to_name(err));
            return;
        }
        s_rx_chan_enabled = true;
    }
    xQueueReset(s_rx_evt_queue);
    xSemaphoreTake(s_rx_done_sem, 0);
    s_rx_running = true;
    xTaskCreatePinnedToCore(rx_task, "hw_ir_rx", 4096, NULL,
                            tskIDLE_PRIORITY + 4, &s_rx_task, 1);
    ESP_LOGI(TAG, "RX start: GPIO%d, %u-symbol buffer, EOF=%lu ms",
             IR_RX_GPIO, IR_RX_MAX_SYMBOLS, (unsigned long)(IR_RX_TIMEOUT_NS / 1000000ULL));
}

static void rx_stop_common(void)
{
    if(!s_rx_running) return;
    s_rx_running = false;
    rx_evt_t sentinel = { .symbols = NULL, .count = 0 };
    xQueueSend(s_rx_evt_queue, &sentinel, 0);

    if(s_rx_done_sem) xSemaphoreTake(s_rx_done_sem, pdMS_TO_TICKS(500));

    if(s_rx_chan && s_rx_chan_enabled) {
        esp_err_t err = rmt_disable(s_rx_chan);
        if(err != ESP_OK) ESP_LOGW(TAG, "rmt_disable: %s", esp_err_to_name(err));
        s_rx_chan_enabled = false;
    }
    ESP_LOGI(TAG, "RX stop");
}

void hw_ir_rx_start(hw_ir_rx_cb_t cb, void *ctx)
{
    if(!s_inited || s_rx_running) return;
    s_rx_user_cb  = cb;
    s_rx_user_ctx = ctx;
    s_rx_edge_cb  = NULL;
    s_rx_edge_ctx = NULL;
    rx_start_common();
}

void hw_ir_rx_stop(void)
{
    rx_stop_common();
    s_rx_user_cb  = NULL;
    s_rx_user_ctx = NULL;
}

void hw_ir_rx_edge_start(hw_ir_rx_edge_cb_t cb, void *ctx)
{
    if(!s_inited || s_rx_running) return;
    s_rx_edge_cb  = cb;
    s_rx_edge_ctx = ctx;
    s_rx_user_cb  = NULL;
    s_rx_user_ctx = NULL;
    rx_start_common();
}

void hw_ir_rx_edge_stop(void)
{
    rx_stop_common();
    s_rx_edge_cb  = NULL;
    s_rx_edge_ctx = NULL;
}
