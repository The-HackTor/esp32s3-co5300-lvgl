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

/* True if the last TX completed within the given microsecond window.
 * Use to gate RX frames that are likely just self-echo from our own LEDs
 * leaking into the TSOP. window_us=100000 (100 ms) is a safe default. */
bool hw_ir_tx_was_recent_us(int64_t window_us);

/* Arm a one-shot ESP_LOGI on the next hw_ir_send_raw call: dumps n_timings,
 * carrier_hz arg, s_carrier_hz_active after apply, apply_carrier err, and
 * the first 8 timings. Self-disarms after one log. Bench-triage only. */
void hw_ir_log_next_send(void);

#endif
