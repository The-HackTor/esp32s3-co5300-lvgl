/* In-RAM stub for the SubGHz signal storage layer. */

#include "signal_store.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_attr.h"

static const char *TAG = "signal_store";

struct slot_rec {
    bool occupied;
    char name[SIGNAL_NAME_MAX];
    struct signal_meta meta;
    int32_t samples[SIGNAL_STORE_MAX_SAVE_SAMPLES];
    uint16_t sample_count;
};

EXT_RAM_BSS_ATTR static struct slot_rec s_slots[SIGNAL_STORE_MAX_SLOTS];

void signal_store_slot_path(uint8_t slot, char *out, size_t len)
{
    if (!out || !len) return;
    snprintf(out, len, "/ram/subghz/saved/slot_%u.raw", slot);
}

int signal_store_save_file(uint8_t slot, const char *name,
                           const struct signal_meta *meta,
                           const char *src_file_path)
{
    ESP_LOGW(TAG, "STUB: save_file slot=%u (src=%s)", slot,
             src_file_path ? src_file_path : "<null>");
    if (slot >= SIGNAL_STORE_MAX_SLOTS || !name || !meta) return -1;
    s_slots[slot].occupied = true;
    strncpy(s_slots[slot].name, name, sizeof(s_slots[slot].name) - 1);
    s_slots[slot].name[sizeof(s_slots[slot].name) - 1] = '\0';
    s_slots[slot].meta = *meta;
    s_slots[slot].sample_count = 0;
    return 0;
}

int signal_store_init(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    ESP_LOGI(TAG, "in-RAM signal store ready (%d slots)", SIGNAL_STORE_MAX_SLOTS);
    return 0;
}

int signal_store_save(uint8_t slot, const char *name,
                      const struct signal_meta *meta,
                      const int32_t *samples, uint16_t count)
{
    if (slot >= SIGNAL_STORE_MAX_SLOTS || !name || !meta) return -1;
    s_slots[slot].occupied = true;
    strncpy(s_slots[slot].name, name, sizeof(s_slots[slot].name) - 1);
    s_slots[slot].name[sizeof(s_slots[slot].name) - 1] = '\0';
    s_slots[slot].meta = *meta;
    uint16_t n = count > SIGNAL_STORE_MAX_SAVE_SAMPLES
                    ? SIGNAL_STORE_MAX_SAVE_SAMPLES : count;
    if (samples && n) memcpy(s_slots[slot].samples, samples, n * sizeof(int32_t));
    s_slots[slot].sample_count = n;
    return 0;
}

int signal_store_load(uint8_t slot, char *name, size_t name_len,
                      struct signal_meta *meta,
                      int32_t *samples, uint16_t max_count)
{
    if (slot >= SIGNAL_STORE_MAX_SLOTS || !s_slots[slot].occupied) return -1;
    if (name && name_len) {
        strncpy(name, s_slots[slot].name, name_len - 1);
        name[name_len - 1] = '\0';
    }
    if (meta) *meta = s_slots[slot].meta;
    if (samples && max_count) {
        uint16_t n = s_slots[slot].sample_count < max_count
                        ? s_slots[slot].sample_count : max_count;
        memcpy(samples, s_slots[slot].samples, n * sizeof(int32_t));
        return n;
    }
    return 0;
}

int signal_store_delete(uint8_t slot)
{
    if (slot >= SIGNAL_STORE_MAX_SLOTS) return -1;
    s_slots[slot].occupied = false;
    return 0;
}

int signal_store_list(void (*print_fn)(uint8_t slot, const char *name))
{
    int n = 0;
    for (uint8_t i = 0; i < SIGNAL_STORE_MAX_SLOTS; i++) {
        if (s_slots[i].occupied) {
            if (print_fn) print_fn(i, s_slots[i].name);
            n++;
        }
    }
    return n;
}

int signal_store_count(void)
{
    int n = 0;
    for (uint8_t i = 0; i < SIGNAL_STORE_MAX_SLOTS; i++) {
        if (s_slots[i].occupied) n++;
    }
    return n;
}

bool signal_store_has(uint8_t slot)
{
    return slot < SIGNAL_STORE_MAX_SLOTS && s_slots[slot].occupied;
}

int signal_store_find_free_slot(void)
{
    for (uint8_t i = 0; i < SIGNAL_STORE_MAX_SLOTS; i++) {
        if (!s_slots[i].occupied) return i;
    }
    return -1;
}

void signal_store_auto_name(char *name, size_t len,
                            const struct signal_meta *meta)
{
    if (!name || !len) return;
    if (meta && meta->protocol[0]) {
        snprintf(name, len, "%s_%u", meta->protocol, (unsigned)meta->freq_khz);
    } else {
        snprintf(name, len, "signal_%u", meta ? (unsigned)meta->freq_khz : 0);
    }
}
