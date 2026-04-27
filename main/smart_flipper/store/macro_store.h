#ifndef MACRO_STORE_H
#define MACRO_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "ir_store.h"

#define IR_MACRO_DIR        "/sdcard/ir/macros"
#define IR_MACRO_NAME_MAX   32
#define IR_MACRO_PATH_MAX   96
#define IR_MACRO_STEP_MAX   32

typedef struct {
    char     remote[IR_REMOTE_NAME_MAX];
    char     button[IR_REMOTE_NAME_MAX];
    uint32_t delay_ms;
    uint8_t  repeat;        /* >=1; default 1; legacy files load as 1 */
} IrMacroStep;

typedef struct {
    char         name[IR_MACRO_NAME_MAX];
    char         path[IR_MACRO_PATH_MAX];
    IrMacroStep *steps;
    size_t       step_count;
    size_t       step_cap;
} IrMacro;

esp_err_t macro_store_init(void);

esp_err_t macro_store_list(char (*out_names)[IR_MACRO_NAME_MAX],
                           size_t cap, size_t *out_count);
esp_err_t macro_store_path(char *out, size_t out_len, const char *name);

esp_err_t macro_init(IrMacro *m, const char *name);
esp_err_t macro_load(IrMacro *out, const char *path);
esp_err_t macro_save(const IrMacro *in);
esp_err_t macro_delete_file(const IrMacro *m);
void      macro_free(IrMacro *m);

esp_err_t macro_append_step(IrMacro *m, const IrMacroStep *s);
esp_err_t macro_delete_step(IrMacro *m, size_t idx);
esp_err_t macro_move_step(IrMacro *m, size_t from, size_t to);

#endif
