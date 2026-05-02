#ifndef HW_IR_H
#define HW_IR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/*
 * IR HAL: TX=GPIO16 (DMN2058UW NMOS, 3x VSMY14940), RX=GPIO17 (TSOP14438,
 * active-low, 10k pull-up). Hardware-modulated carrier via RMT.
 * 38 kHz only on RX -- 40 kHz SIRC / 36 kHz RC5 are TX-correct but
 * won't demodulate on this TSOP variant.
 *
 * Raw-timing buffers alternate mark/space in microseconds; index 0 is mark.
 */

void hw_ir_init(void);

/* Blocks until burst done. carrier_hz=0 disables modulator. Each
 * duration is clamped to the RMT 15-bit field (32767 us). */
esp_err_t hw_ir_send_raw(const uint16_t *timings, size_t n_timings, uint32_t carrier_hz);

/*
 * Hold-to-repeat. First burst fires synchronously so a tap still TXes once.
 * `repeat_timings` is the protocol's short-repeat (NEC/NECext/NEC42/
 * Samsung32) or the full frame for protocols without one (RC5/RC6/Pioneer/
 * RCA). NULL falls back to re-firing `timings`. Buffers are copied.
 *
 * `min_repeats` is the minimum total frame count (including the initial)
 * the worker will send before honoring stop. SIRC needs 3 — Sony TVs
 * silently drop bursts shorter than that. 0 or 1 means no minimum.
 */
esp_err_t hw_ir_send_repeat_start(const uint16_t *timings, size_t n_timings,
                                  const uint16_t *repeat_timings, size_t repeat_n_timings,
                                  uint32_t carrier_hz, uint32_t period_ms,
                                  uint32_t min_repeats);
void      hw_ir_send_repeat_stop(void);

/*
 * RX runs in a private FreeRTOS task (NOT the LVGL task). Callbacks
 * MUST NOT touch LVGL -- forward via queue + lv_timer drain.
 * The TSOP inverts (idle HIGH); driver re-inverts so callers see
 * mark-first like the LED was driven.
 */
typedef void (*hw_ir_rx_cb_t)(const uint16_t *timings, size_t n_timings, void *ctx);

void hw_ir_rx_start(hw_ir_rx_cb_t cb, void *ctx);
void hw_ir_rx_stop(void);

/* Per-edge variant: matches Flipper furi_hal_infrared so decoders see
 * real polarity rather than parity-inferred. eof flags RMT EOF timeout. */
typedef void (*hw_ir_rx_edge_cb_t)(bool level, uint32_t duration_us, bool eof, void *ctx);

void hw_ir_rx_edge_start(hw_ir_rx_edge_cb_t cb, void *ctx);
void hw_ir_rx_edge_stop(void);

/* Self-echo gate -- our own LEDs leak into the TSOP for ~tens of ms. */
bool hw_ir_tx_was_recent_us(int64_t window_us);

/* Bench triage: one-shot dump on the next send_raw, then self-disarms. */
void hw_ir_log_next_send(void);

/* Rebuild RMT TX channel with inverted output. Safe at any time. */
esp_err_t hw_ir_set_invert(bool invert);

/*
 * Async TX engine: worker on core 1 owns a request queue. Use this
 * whenever a caller would otherwise block the LVGL task. on_done runs
 * on the worker -- MUST NOT call LVGL.
 */
typedef enum {
    HW_IR_TX_DONE      = 0,
    HW_IR_TX_CANCELLED = 1,
    HW_IR_TX_ERROR     = 2,
} HwIrTxResult;

typedef void (*hw_ir_tx_done_cb_t)(uint32_t id, HwIrTxResult result, void *ctx);

typedef struct {
    const uint16_t   *timings;     /* copied on submit */
    size_t            n_timings;
    uint32_t          carrier_hz;
    uint8_t           repeat;      /* 1..32 */
    uint16_t          gap_ms;
    hw_ir_tx_done_cb_t on_done;
    void             *ctx;
} HwIrTxRequest;

/* Non-zero id on success, 0 on failure. */
uint32_t hw_ir_tx_submit(const HwIrTxRequest *req);

/* Cancellation point is the next inter-frame gap (bounded by gap_ms).
 * on_done fires with CANCELLED. Idempotent. */
void hw_ir_tx_cancel(uint32_t id);

void hw_ir_tx_cancel_all(uint32_t timeout_ms);

void hw_ir_quiesce_for_deep_sleep(void);

#endif
