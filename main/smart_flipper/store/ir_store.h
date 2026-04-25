#ifndef IR_STORE_H
#define IR_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define IR_STORE_DIR              "/sdcard/ir"
#define IR_REMOTE_NAME_MAX        32
#define IR_REMOTE_PATH_MAX        80
#define IR_PROTOCOL_NAME_MAX      24

typedef enum {
    INFRARED_SIGNAL_PARSED = 0,
    INFRARED_SIGNAL_RAW,
} InfraredSignalType;

typedef struct {
    char     protocol[IR_PROTOCOL_NAME_MAX];
    uint32_t address;
    uint32_t command;
} InfraredSignalParsed;

typedef struct {
    uint32_t  freq_hz;
    uint16_t  duty_pct;
    uint16_t *timings;
    size_t    n_timings;
} InfraredSignalRaw;

typedef struct {
    InfraredSignalType type;
    union {
        InfraredSignalParsed parsed;
        InfraredSignalRaw    raw;
    };
} InfraredSignal;

typedef struct {
    char           name[IR_REMOTE_NAME_MAX];
    InfraredSignal signal;
} IrButton;

typedef struct {
    char      name[IR_REMOTE_NAME_MAX];
    char      path[IR_REMOTE_PATH_MAX];
    IrButton *buttons;
    size_t    button_count;
    size_t    button_cap;
    bool      dirty;
} IrRemote;

esp_err_t ir_store_init(void);
esp_err_t ir_store_remote_path(char *out, size_t out_len, const char *name);
esp_err_t ir_store_list_remotes(char (*out_names)[IR_REMOTE_NAME_MAX],
                                size_t cap, size_t *out_count);

esp_err_t ir_remote_init(IrRemote *r, const char *name);
esp_err_t ir_remote_load(IrRemote *out, const char *path);
esp_err_t ir_remote_save(const IrRemote *in);
esp_err_t ir_remote_append_button(IrRemote *r, const IrButton *btn);
esp_err_t ir_remote_delete_button(IrRemote *r, size_t idx);
esp_err_t ir_remote_rename_button(IrRemote *r, size_t idx, const char *new_name);
void      ir_remote_free(IrRemote *r);

void      ir_button_free(IrButton *btn);
esp_err_t ir_button_dup(IrButton *dst, const IrButton *src);

#endif
