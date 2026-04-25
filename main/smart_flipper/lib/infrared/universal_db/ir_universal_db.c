#include "ir_universal_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#define TAG "ir_universal_db"

extern const char _binary_tv_ir_start[]        asm("_binary_tv_ir_start");
extern const char _binary_tv_ir_end[]          asm("_binary_tv_ir_end");
extern const char _binary_ac_ir_start[]        asm("_binary_ac_ir_start");
extern const char _binary_ac_ir_end[]          asm("_binary_ac_ir_end");
extern const char _binary_audio_ir_start[]     asm("_binary_audio_ir_start");
extern const char _binary_audio_ir_end[]       asm("_binary_audio_ir_end");
extern const char _binary_projector_ir_start[] asm("_binary_projector_ir_start");
extern const char _binary_projector_ir_end[]   asm("_binary_projector_ir_end");

typedef struct {
    char      name[IR_REMOTE_NAME_MAX];
    uint16_t *button_indices;
    size_t    count;
    size_t    cap;
} UniqueButton;

typedef struct {
    IrRemote      remote;
    UniqueButton *buttons;
    size_t        button_count;
    bool          loaded;
} CategoryDb;

static CategoryDb s_dbs[IR_UNIVERSAL_CAT_COUNT];

static UniqueButton *find_or_create(CategoryDb *db, const char *name)
{
    for(size_t i = 0; i < db->button_count; i++) {
        if(strcmp(db->buttons[i].name, name) == 0) return &db->buttons[i];
    }
    UniqueButton *grown = realloc(db->buttons,
                                  (db->button_count + 1) * sizeof(UniqueButton));
    if(!grown) return NULL;
    db->buttons = grown;
    UniqueButton *ub = &db->buttons[db->button_count++];
    memset(ub, 0, sizeof(*ub));
    snprintf(ub->name, sizeof(ub->name), "%s", name);
    return ub;
}

static esp_err_t append_index(UniqueButton *ub, uint16_t idx)
{
    if(ub->count == ub->cap) {
        size_t new_cap = ub->cap ? ub->cap * 2 : 4;
        uint16_t *grown = realloc(ub->button_indices, new_cap * sizeof(uint16_t));
        if(!grown) return ESP_ERR_NO_MEM;
        ub->button_indices = grown;
        ub->cap = new_cap;
    }
    ub->button_indices[ub->count++] = idx;
    return ESP_OK;
}

static void build_index(CategoryDb *db)
{
    for(size_t i = 0; i < db->remote.button_count && i < UINT16_MAX; i++) {
        UniqueButton *ub = find_or_create(db, db->remote.buttons[i].name);
        if(!ub) break;
        append_index(ub, (uint16_t)i);
    }
}

esp_err_t ir_universal_db_init(void)
{
    static const struct {
        const char *name;
        const char *start;
        const char *end;
    } blobs[IR_UNIVERSAL_CAT_COUNT] = {
        { "tv",        _binary_tv_ir_start,        _binary_tv_ir_end        },
        { "ac",        _binary_ac_ir_start,        _binary_ac_ir_end        },
        { "audio",     _binary_audio_ir_start,     _binary_audio_ir_end     },
        { "projector", _binary_projector_ir_start, _binary_projector_ir_end },
    };

    for(int c = 0; c < IR_UNIVERSAL_CAT_COUNT; c++) {
        size_t len = (size_t)(blobs[c].end - blobs[c].start);
        esp_err_t err = ir_remote_load_blob(&s_dbs[c].remote, blobs[c].name,
                                            blobs[c].start, len);
        if(err != ESP_OK) {
            ESP_LOGW(TAG, "load %s: %s", blobs[c].name, esp_err_to_name(err));
            continue;
        }
        build_index(&s_dbs[c]);
        s_dbs[c].loaded = true;
        ESP_LOGI(TAG, "%s: %u entries, %u unique names",
                 blobs[c].name,
                 (unsigned)s_dbs[c].remote.button_count,
                 (unsigned)s_dbs[c].button_count);
    }
    return ESP_OK;
}

size_t ir_universal_db_button_count(IrUniversalCategory cat)
{
    if((unsigned)cat >= IR_UNIVERSAL_CAT_COUNT) return 0;
    return s_dbs[cat].button_count;
}

const char *ir_universal_db_button_name(IrUniversalCategory cat, size_t idx)
{
    if((unsigned)cat >= IR_UNIVERSAL_CAT_COUNT) return NULL;
    CategoryDb *db = &s_dbs[cat];
    if(idx >= db->button_count) return NULL;
    return db->buttons[idx].name;
}

size_t ir_universal_db_signal_count(IrUniversalCategory cat, size_t button_idx)
{
    if((unsigned)cat >= IR_UNIVERSAL_CAT_COUNT) return 0;
    CategoryDb *db = &s_dbs[cat];
    if(button_idx >= db->button_count) return 0;
    return db->buttons[button_idx].count;
}

const IrButton *ir_universal_db_button_signal(IrUniversalCategory cat,
                                              size_t button_idx,
                                              size_t signal_idx)
{
    if((unsigned)cat >= IR_UNIVERSAL_CAT_COUNT) return NULL;
    CategoryDb *db = &s_dbs[cat];
    if(button_idx >= db->button_count) return NULL;
    UniqueButton *ub = &db->buttons[button_idx];
    if(signal_idx >= ub->count) return NULL;
    uint16_t i = ub->button_indices[signal_idx];
    if(i >= db->remote.button_count) return NULL;
    return &db->remote.buttons[i];
}
