#include "ir_universal_index.h"
#include "ir_universal_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#define TAG "ir_universal_index"

typedef struct {
    uint32_t proto_hash;
    uint32_t address;
    uint16_t category;
    uint16_t button_idx;
} IndexEntry;

static IndexEntry *s_entries;
static size_t      s_count;

static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    while(*s) {
        h ^= (uint8_t)*s++;
        h *= 16777619u;
    }
    return h;
}

esp_err_t ir_universal_index_init(void)
{
    if(s_entries) {
        free(s_entries);
        s_entries = NULL;
        s_count = 0;
    }

    size_t total = 0;
    for(int c = 0; c < IR_UNIVERSAL_CAT_COUNT; c++) {
        const IrRemote *r = ir_universal_db_get_remote((IrUniversalCategory)c);
        if(!r) continue;
        for(size_t i = 0; i < r->button_count; i++) {
            if(r->buttons[i].signal.type == INFRARED_SIGNAL_PARSED) total++;
        }
    }
    if(total == 0) return ESP_OK;

    s_entries = malloc(total * sizeof(IndexEntry));
    if(!s_entries) return ESP_ERR_NO_MEM;

    size_t k = 0;
    for(int c = 0; c < IR_UNIVERSAL_CAT_COUNT; c++) {
        const IrRemote *r = ir_universal_db_get_remote((IrUniversalCategory)c);
        if(!r) continue;
        for(size_t i = 0; i < r->button_count && k < total; i++) {
            const IrButton *b = &r->buttons[i];
            if(b->signal.type != INFRARED_SIGNAL_PARSED) continue;
            s_entries[k].proto_hash = fnv1a(b->signal.parsed.protocol);
            s_entries[k].address    = b->signal.parsed.address;
            s_entries[k].category   = (uint16_t)c;
            s_entries[k].button_idx = (uint16_t)i;
            k++;
        }
    }
    s_count = k;
    ESP_LOGI(TAG, "indexed %u parsed entries across categories", (unsigned)s_count);
    return ESP_OK;
}

bool ir_universal_index_match(const char *protocol, uint32_t address,
                              IrUniversalCategory *out_cat,
                              char *out_label, size_t out_label_len)
{
    if(!protocol || !s_entries || s_count == 0) return false;
    uint32_t h = fnv1a(protocol);

    for(size_t i = 0; i < s_count; i++) {
        if(s_entries[i].proto_hash != h)         continue;
        if(s_entries[i].address    != address)   continue;

        IrUniversalCategory cat = (IrUniversalCategory)s_entries[i].category;
        const IrRemote *r = ir_universal_db_get_remote(cat);
        if(!r || s_entries[i].button_idx >= r->button_count) continue;

        if(out_cat) *out_cat = cat;
        if(out_label && out_label_len > 0) {
            snprintf(out_label, out_label_len, "%s",
                     r->buttons[s_entries[i].button_idx].name);
        }
        return true;
    }
    return false;
}
