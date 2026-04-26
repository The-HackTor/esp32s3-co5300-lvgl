#ifndef IR_STORE_H
#define IR_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define IR_STORE_DIR              "/sdcard/ir"
#define IR_UNIVERSAL_DIR          "/sdcard/ir/universal"
#define IR_REMOTE_NAME_MAX        32
#define IR_REMOTE_PATH_MAX        96
#define IR_PROTOCOL_NAME_MAX      24

typedef enum {
    IR_UNIVERSAL_CAT_TV = 0,
    IR_UNIVERSAL_CAT_AC,
    IR_UNIVERSAL_CAT_AUDIO,
    IR_UNIVERSAL_CAT_PROJECTOR,
    IR_UNIVERSAL_CAT_COUNT,
} IrUniversalCategory;

const char *ir_universal_category_dirname(IrUniversalCategory cat);
const char *ir_universal_category_label(IrUniversalCategory cat);

esp_err_t ir_universal_list(IrUniversalCategory cat,
                            char (*out_names)[IR_REMOTE_NAME_MAX],
                            size_t cap, size_t *out_count);
esp_err_t ir_universal_path(IrUniversalCategory cat, const char *name,
                            char *out, size_t out_len);

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

#define IR_HISTORY_PATH        "/sdcard/ir/history.log"
#define IR_HISTORY_MAX_ENTRIES 64

typedef struct {
    int64_t  timestamp_us;
    char     protocol[IR_PROTOCOL_NAME_MAX];
    uint32_t address;
    uint32_t command;
} IrHistoryEntry;

esp_err_t ir_history_append(const IrHistoryEntry *e);
esp_err_t ir_history_read(IrHistoryEntry *out, size_t cap, size_t *out_count);
esp_err_t ir_history_delete(size_t idx);
esp_err_t ir_history_clear(void);

esp_err_t ir_remote_init(IrRemote *r, const char *name);
esp_err_t ir_remote_load(IrRemote *out, const char *path);
esp_err_t ir_remote_load_blob(IrRemote *out, const char *name,
                              const char *buf, size_t buf_len);
esp_err_t ir_remote_save(const IrRemote *in);
esp_err_t ir_remote_append_button(IrRemote *r, const IrButton *btn);
esp_err_t ir_remote_delete_button(IrRemote *r, size_t idx);
esp_err_t ir_remote_rename_button(IrRemote *r, size_t idx, const char *new_name);
esp_err_t ir_remote_move_button(IrRemote *r, size_t from, size_t to);
esp_err_t ir_remote_rename(IrRemote *r, const char *new_name);
esp_err_t ir_remote_delete_file(const IrRemote *r);
void      ir_remote_free(IrRemote *r);

void      ir_button_free(IrButton *btn);
esp_err_t ir_button_dup(IrButton *dst, const IrButton *src);

#endif
