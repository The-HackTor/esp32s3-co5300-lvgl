#ifndef HW_IR_H
#define HW_IR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

/*
 * IR HAL for ESP32-S3 + IR transceiver expansion (J1/J2 header).
 *
 * Hardware (locked from schematic, J2 IR header):
 *   IR_TX = GPIO16 (DMN2058UW NMOS driving 3x VSMY14940 IR LEDs in parallel).
 *           Hardware-modulated 38 kHz carrier via RMT TX.
 *   IR_RX = GPIO17 (TSOP14438 demodulated envelope, active-low, 10k pullup).
 *           38 kHz only -- 40 kHz SIRC and 36 kHz RC5 will TX correctly but
 *           won't RX reliably.
 *
 * Timing values are durations in microseconds. A "raw timing" buffer is
 * an alternating sequence of mark (LED on, modulated by carrier) and space
 * (LED off) durations -- index 0 is mark, index 1 is space, etc. NEC, RC5,
 * Samsung, AC remotes all reduce to such a buffer.
 */

void hw_ir_init(void);

/*
 * Transmit a raw alternating mark/space sequence with hardware-modulated
 * carrier. Blocks until the burst completes. n must be > 0; n_timings is in
 * units of uint16_t entries (NOT bytes). Each duration is 1..32767 us.
 *
 * carrier_hz:
 *   38000  - default (NEC, Samsung, RC6, most ACs)
 *   40000  - SIRC (Sony) -- TX-only on this board
 *   36000  - RC5 -- TX-only on this board
 *   0      - no carrier (raw envelope, mostly for debugging)
 */
esp_err_t hw_ir_send_raw(const uint16_t *timings, size_t n_timings, uint32_t carrier_hz);

/*
 * Hold-to-repeat TX. Spawns a worker that retransmits the burst every
 * period_ms until hw_ir_send_repeat_stop is called. The first burst fires
 * synchronously inside _start so a quick tap still produces one TX.
 *
 * Buffer is copied internally; caller may free immediately. Calling _start
 * while already repeating returns ESP_ERR_INVALID_STATE.
 */
esp_err_t hw_ir_send_repeat_start(const uint16_t *timings, size_t n_timings,
                                  uint32_t carrier_hz, uint32_t period_ms);
void      hw_ir_send_repeat_stop(void);

/*
 * RX: edge-capture into a PSRAM-backed buffer. Each captured frame is a
 * sequence of alternating mark/space durations in microseconds, terminated
 * by an end-of-frame timeout (no edges for ~50 ms). The library decodes
 * timings AS DRIVEN BY THE LED -- the TSOP14438 inverts (idle high, mark =
 * pulled low), but hw_ir.c re-inverts so callers always see mark-first.
 *
 * Callback runs in a private FreeRTOS task (NOT the LVGL task). Implementations
 * must not touch LVGL widgets directly -- xQueueSend to a queue drained by a
 * lv_timer_t on the LVGL side.
 */
typedef void (*hw_ir_rx_cb_t)(const uint16_t *timings, size_t n_timings, void *ctx);

void hw_ir_rx_start(hw_ir_rx_cb_t cb, void *ctx);
void hw_ir_rx_stop(void);

/* Per-edge callback. Called from the rx_task once per (level, duration)
 * edge as captured. level is the actual TSOP-line level; mirrors Flipper's
 * furi_hal_infrared rx_callback contract so the decoder receives real
 * polarity instead of parity-inferred values. eof is true when the burst
 * ended (RMT EOF timeout) and the worker should run end-of-frame
 * finalization. */
typedef void (*hw_ir_rx_edge_cb_t)(bool level, uint32_t duration_us, bool eof, void *ctx);

void hw_ir_rx_edge_start(hw_ir_rx_edge_cb_t cb, void *ctx);
void hw_ir_rx_edge_stop(void);

/* True if the last TX completed within the given microsecond window.
 * Use to gate RX frames that are likely just self-echo from our own LEDs
 * leaking into the TSOP. window_us=100000 (100 ms) is a safe default. */
bool hw_ir_tx_was_recent_us(int64_t window_us);

/* Arm a one-shot ESP_LOGI on the next hw_ir_send_raw call: dumps n_timings,
 * carrier_hz arg, s_carrier_hz_active after apply, apply_carrier err, and
 * the first 8 timings. Self-disarms after one log. Bench-triage only. */
void hw_ir_log_next_send(void);

/* Toggle the IR_TX GPIO output inversion. Tears down and rebuilds the RMT
 * TX channel under s_tx_mtx; safe to call at any time. Use to flip carrier
 * polarity at the LED line when the gate-driver path inverts unexpectedly. */
esp_err_t hw_ir_set_invert(bool invert);

/*
 * Async TX engine. A single worker task (pinned to core 1) owns a request
 * queue. Submit a transmit, return immediately, get notified via callback
 * when the worker finishes / is cancelled.
 *
 * Use this instead of hw_ir_send_raw whenever the caller would otherwise
 * block the LVGL task or a UI-driven runner. The brute scene's per-step
 * burst uses this path; cancellation is bounded.
 *
 * Lifecycle:
 *   hw_ir_init()                 -- starts worker on first call
 *   id = hw_ir_tx_submit(req)    -- copies timings; worker fires when ready
 *   hw_ir_tx_cancel(id)          -- soft-cancel a queued or in-flight burst
 *   hw_ir_tx_cancel_all()        -- drain queue + abandon current burst
 *
 * The on_done callback runs on the worker task; it MUST NOT call LVGL.
 * Forward to the LVGL side via xQueueSend / lv_async / a drain timer.
 */
typedef enum {
    HW_IR_TX_DONE      = 0,
    HW_IR_TX_CANCELLED = 1,
    HW_IR_TX_ERROR     = 2,
} HwIrTxResult;

typedef void (*hw_ir_tx_done_cb_t)(uint32_t id, HwIrTxResult result, void *ctx);

typedef struct {
    const uint16_t   *timings;     /* copied internally on submit */
    size_t            n_timings;
    uint32_t          carrier_hz;
    uint8_t           repeat;      /* 1..32 */
    uint16_t          gap_ms;      /* between repeats */
    hw_ir_tx_done_cb_t on_done;
    void             *ctx;
} HwIrTxRequest;

/* Returns a non-zero submission id on success, 0 on failure. */
uint32_t hw_ir_tx_submit(const HwIrTxRequest *req);

/* Soft-cancel: matches by id. If the burst is currently firing, cancellation
 * takes effect at the next inter-frame boundary (within gap_ms). The on_done
 * callback fires with HW_IR_TX_CANCELLED. Idempotent. */
void hw_ir_tx_cancel(uint32_t id);

/* Drain everything: queued submissions are cancelled (each fires on_done
 * with CANCELLED before being dropped). The currently-firing burst is also
 * cancelled at next gap boundary. Blocks up to timeout_ms for the worker
 * to ack idle. */
void hw_ir_tx_cancel_all(uint32_t timeout_ms);

#endif
