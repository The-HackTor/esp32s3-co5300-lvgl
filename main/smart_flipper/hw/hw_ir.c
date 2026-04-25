#include "hw_ir.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "esp_attr.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "hw_ir";

#define IR_TX_GPIO          5
#define IR_RX_GPIO          3
#define IR_RMT_RESOLUTION   1000000U   /* 1 us per tick -- timings are direct microseconds */
#define IR_TX_MEM_SYMBOLS   64
#define IR_RX_MEM_SYMBOLS   128        /* non-DMA on S3 supports up to a few hundred */
#define IR_TX_QUEUE_DEPTH   4
#define IR_CARRIER_DUTY     0.33f      /* per VSMY14940 datasheet sweet spot */

#define IR_MAX_TIMINGS      1024       /* one signal -- NEC ~67, AC frames ~600 */
#define IR_RX_MAX_SYMBOLS   512        /* one captured frame, 1024 alternating edges */

/* End-of-frame: 50 ms of no edge ends the capture. Plenty above the longest
 * inter-frame gap of any AC remote (~20 ms). */
#define IR_RX_TIMEOUT_NS    50000000ULL
/* Anything shorter than 100 us is noise (TSOP can't resolve faster anyway). */
#define IR_RX_MIN_NS        100000ULL
/* Cap individual phase length at 30 ms; longer gaps end the frame. */
#define IR_RX_MAX_NS        30000000ULL

static rmt_channel_handle_t  s_tx_chan;
static rmt_encoder_handle_t  s_copy_encoder;
static uint32_t              s_carrier_hz_active;
static bool                  s_inited;

/* RX state */
static rmt_channel_handle_t  s_rx_chan;
static rmt_symbol_word_t    *s_rx_buf;       /* IDF fills caller-provided buffer */
static QueueHandle_t         s_rx_evt_queue;
static TaskHandle_t          s_rx_task;
static hw_ir_rx_cb_t         s_rx_user_cb;
static void                 *s_rx_user_ctx;
static bool                  s_rx_running;

typedef struct {
    rmt_symbol_word_t *symbols;
    size_t             count;
} rx_evt_t;

static esp_err_t apply_carrier(uint32_t carrier_hz)
{
    if(carrier_hz == s_carrier_hz_active) return ESP_OK;

    rmt_carrier_config_t cfg = {
        .frequency_hz     = carrier_hz,
        .duty_cycle       = IR_CARRIER_DUTY,
        .flags.polarity_active_low = false,
    };
    if(carrier_hz == 0) {
        /* IDF rejects frequency_hz==0; disable by passing flags.disable. */
        cfg.frequency_hz = 38000;
        /* No standard "disable" flag in v6.1 -- workaround: pass 0 duty.
         * In practice we always send with carrier; the 0 path is debug only. */
        cfg.duty_cycle = 0.0f;
    }
    esp_err_t err = rmt_apply_carrier(s_tx_chan, &cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_apply_carrier(%u): %s", (unsigned)carrier_hz, esp_err_to_name(err));
        return err;
    }
    s_carrier_hz_active = carrier_hz;
    return ESP_OK;
}

void hw_ir_init(void)
{
    if(s_inited) return;

    const rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = IR_TX_GPIO,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = IR_RMT_RESOLUTION,
        .mem_block_symbols = IR_TX_MEM_SYMBOLS,
        .trans_queue_depth = IR_TX_QUEUE_DEPTH,
        .flags.invert_out  = false,
        .flags.with_dma    = false,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_tx_chan));

    /* Copy encoder: feeds caller-provided rmt_symbol_word_t array straight
     * to the channel's TX hardware. The carrier is applied by the channel
     * (hardware-modulated), so symbols carry plain mark/space durations.
     * IDF v6.1 made rmt_copy_encoder_config_t an empty struct -- {0} triggers
     * -Werror=excess-initializers, so use the empty-aggregate form. */
    const rmt_copy_encoder_config_t enc_cfg = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&enc_cfg, &s_copy_encoder));

    ESP_ERROR_CHECK(apply_carrier(38000));
    ESP_ERROR_CHECK(rmt_enable(s_tx_chan));

    s_inited = true;
    ESP_LOGI(TAG, "init: TX=GPIO%d, %u Hz tick, default carrier=38 kHz",
             IR_TX_GPIO, (unsigned)IR_RMT_RESOLUTION);
}

esp_err_t hw_ir_send_raw(const uint16_t *timings, size_t n_timings, uint32_t carrier_hz)
{
    if(!s_inited)                       return ESP_ERR_INVALID_STATE;
    if(timings == NULL || n_timings == 0) return ESP_ERR_INVALID_ARG;
    if(n_timings > IR_MAX_TIMINGS)      return ESP_ERR_INVALID_SIZE;

    esp_err_t err = apply_carrier(carrier_hz ? carrier_hz : 38000);
    if(err != ESP_OK) return err;

    /* Pack pairs of (mark, space) durations into rmt_symbol_word_t. Each
     * symbol holds two phases (level0+duration0, level1+duration1). For an
     * odd-count buffer (final mark with no trailing space) we pad with a
     * 0-duration LOW phase so RMT terminates cleanly. */
    const size_t n_symbols = (n_timings + 1) / 2;
    rmt_symbol_word_t symbols[(IR_MAX_TIMINGS + 1) / 2];

    for(size_t i = 0; i < n_symbols; i++) {
        const size_t i0 = i * 2;
        const size_t i1 = i0 + 1;
        symbols[i].level0    = 1;            /* mark = LED on (carrier modulated) */
        symbols[i].duration0 = timings[i0];
        if(i1 < n_timings) {
            symbols[i].level1    = 0;        /* space = LED off */
            symbols[i].duration1 = timings[i1];
        } else {
            symbols[i].level1    = 0;
            symbols[i].duration1 = 0;        /* terminator */
        }
    }

    const rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
        .flags.eot_level = 0,                /* idle LOW after burst */
    };

    err = rmt_transmit(s_tx_chan, s_copy_encoder, symbols,
                       n_symbols * sizeof(symbols[0]), &tx_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_transmit: %s", esp_err_to_name(err));
        return err;
    }
    return rmt_tx_wait_all_done(s_tx_chan, pdMS_TO_TICKS(500));
}

/* ---- RX ---- */

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

    /* Buffer for caller-visible timing list, populated per frame. */
    static uint16_t timings[IR_RX_MAX_SYMBOLS * 2];

    /* Prime the channel. */
    ESP_ERROR_CHECK(rmt_receive(s_rx_chan, s_rx_buf,
                                IR_RX_MAX_SYMBOLS * sizeof(rmt_symbol_word_t),
                                &rcv_cfg));

    while(s_rx_running) {
        rx_evt_t evt;
        if(xQueueReceive(s_rx_evt_queue, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        /* Convert symbols (mark/space pairs) to a flat timing list. The
         * TSOP idles HIGH and pulls LOW for marks; we don't filter on
         * level here -- the alternating pattern is what callers care
         * about. Stop at the first 0-duration phase (RMT terminator). */
        size_t out_n = 0;
        for(size_t i = 0; i < evt.count && out_n + 1 < sizeof(timings)/sizeof(timings[0]); i++) {
            if(evt.symbols[i].duration0 == 0) break;
            timings[out_n++] = evt.symbols[i].duration0;
            if(evt.symbols[i].duration1 == 0) break;
            timings[out_n++] = evt.symbols[i].duration1;
        }

        if(s_rx_user_cb && out_n > 0) {
            s_rx_user_cb(timings, out_n, s_rx_user_ctx);
        }

        /* Re-arm. rmt_receive must be called again after each completion. */
        rmt_receive(s_rx_chan, s_rx_buf,
                    IR_RX_MAX_SYMBOLS * sizeof(rmt_symbol_word_t),
                    &rcv_cfg);
    }

    s_rx_task = NULL;
    vTaskDelete(NULL);
}

void hw_ir_rx_start(hw_ir_rx_cb_t cb, void *ctx)
{
    if(!s_inited || s_rx_running) return;

    if(s_rx_chan == NULL) {
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

        ESP_ERROR_CHECK(rmt_enable(s_rx_chan));
    }

    s_rx_user_cb  = cb;
    s_rx_user_ctx = ctx;
    s_rx_running  = true;

    xTaskCreatePinnedToCore(rx_task, "hw_ir_rx", 4096, NULL,
                            tskIDLE_PRIORITY + 4, &s_rx_task, 1);
    ESP_LOGI(TAG, "RX start: GPIO%d, %u-symbol buffer, EOF=%lu ms",
             IR_RX_GPIO, IR_RX_MAX_SYMBOLS, (unsigned long)(IR_RX_TIMEOUT_NS / 1000000ULL));
}

void hw_ir_rx_stop(void)
{
    if(!s_rx_running) return;
    s_rx_running = false;
    /* The task is blocked on the queue; push a sentinel to wake it. */
    rx_evt_t sentinel = { .symbols = NULL, .count = 0 };
    xQueueSend(s_rx_evt_queue, &sentinel, 0);
    s_rx_user_cb  = NULL;
    s_rx_user_ctx = NULL;
}
