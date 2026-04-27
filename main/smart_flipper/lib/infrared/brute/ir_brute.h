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
esp_err_t ir_brute_step_send(const IrBruteContext *bc, size_t idx);
esp_err_t ir_brute_step_to_button(const IrBruteContext *bc, size_t idx, IrButton *out);

#endif
