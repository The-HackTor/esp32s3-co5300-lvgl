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
    uint32_t command;
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
            s_entries[k].command    = b->signal.parsed.command;
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
                              uint32_t command,
                              IrUniversalCategory *out_cat,
                              char *out_label, size_t out_label_len,
                              IrMatchConfidence *out_confidence,
                              uint16_t *out_group_size)
{
    if(!protocol || !s_entries || s_count == 0) return false;
    uint32_t h = fnv1a(protocol);

    /* Single pass: count all (proto, address) hits and remember the best
     * one. Exact (proto, address, command) match wins; otherwise first
     * group hit. group_size is the number of buttons in the matching
     * remote-group -- higher count means the captured signal sits in a
     * dense neighborhood of related buttons, which is a stronger brand
     * fingerprint than a one-off coincidence. */
    size_t  group_size = 0;
    ssize_t exact_idx  = -1;
    ssize_t group_idx  = -1;

    for(size_t i = 0; i < s_count; i++) {
        if(s_entries[i].proto_hash != h)       continue;
        if(s_entries[i].address    != address) continue;
        group_size++;
        if(group_idx < 0) group_idx = (ssize_t)i;
        if(exact_idx < 0 && s_entries[i].command == command) exact_idx = (ssize_t)i;
    }

    if(group_size == 0) return false;

    ssize_t pick = (exact_idx >= 0) ? exact_idx : group_idx;
    IrUniversalCategory cat = (IrUniversalCategory)s_entries[pick].category;
    const IrRemote *r = ir_universal_db_get_remote(cat);
    if(!r || s_entries[pick].button_idx >= r->button_count) return false;

    if(out_cat) *out_cat = cat;
    if(out_label && out_label_len > 0) {
        snprintf(out_label, out_label_len, "%s",
                 r->buttons[s_entries[pick].button_idx].name);
    }
    if(out_confidence) {
        *out_confidence = (exact_idx >= 0) ? IR_MATCH_EXACT : IR_MATCH_GROUP;
    }
    if(out_group_size) {
        *out_group_size = (uint16_t)(group_size > UINT16_MAX ? UINT16_MAX : group_size);
    }
    return true;
}
