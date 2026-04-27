#ifndef IR_BRUTE_H
#define IR_BRUTE_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "store/ir_store.h"

typedef struct {
    IrUniversalCategory cat;
    int                 button_idx;
    size_t              db_count;
    size_t              brand_count;
} IrBruteContext;

typedef struct {
    bool        is_brand;
    const char *brand_name;
    const char *protocol;
    const char *button_label;
} IrBruteStepInfo;

void      ir_brute_init(IrBruteContext *bc, IrUniversalCategory cat, int button_idx);
size_t    ir_brute_total(const IrBruteContext *bc);
bool      ir_brute_step_info(const IrBruteContext *bc, size_t idx, IrBruteStepInfo *out);

/* Encode-only: returns a heap-owned timings buffer the caller must free.
 * Also returns:
 *   out_min_repeat -- protocol's min repeat (NEC=1, SIRC=3, Pioneer=2, ...)
 *   out_silence_ms -- protocol's native inter-frame silence_time. Flipper
 *     uses this between frame repeats (NEC=110, SIRC=10, Samsung=145,
 *     RC5/6=27, Pioneer=26, Kaseikyo=130, RCA=8). RAW/brand fall back to 110.
 * Use this with hw_ir_tx_submit so the LVGL task never blocks on encode+TX. */
esp_err_t ir_brute_step_encode(const IrBruteContext *bc, size_t idx,
                               uint16_t **out_timings, size_t *out_n,
                               uint32_t *out_freq_hz, uint8_t *out_min_repeat,
                               uint16_t *out_silence_ms);

/* Sync convenience: encode + fire `repeat` times via hw_ir_send_raw. Kept for
 * legacy callers; new code should prefer encode + hw_ir_tx_submit. */
esp_err_t ir_brute_step_send(const IrBruteContext *bc, size_t idx, uint8_t repeat);

esp_err_t ir_brute_step_to_button(const IrBruteContext *bc, size_t idx, IrButton *out);

/* Arm a one-shot ESP_LOGI on the next ir_brute_step_encode call. */
void      ir_brute_log_next_send(void);

#endif
