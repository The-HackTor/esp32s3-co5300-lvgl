#include "card_store.h"
#include <string.h>
#include "esp_log.h"
#include "esp_attr.h"

static const char *TAG = "card_store";

struct slot_rec {
    bool occupied;
    char name[32];
    struct mfc_dump dump;
};

EXT_RAM_BSS_ATTR static struct slot_rec s_slots[CARD_STORE_MAX_SLOTS];

int card_store_init(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    ESP_LOGI(TAG, "in-RAM card store ready (%d slots)", CARD_STORE_MAX_SLOTS);
    return 0;
}

int card_store_save(uint8_t slot, const char *name, const struct mfc_dump *dump)
{
    if (slot >= CARD_STORE_MAX_SLOTS || !name || !dump) return -1;
    s_slots[slot].occupied = true;
    strncpy(s_slots[slot].name, name, sizeof(s_slots[slot].name) - 1);
    s_slots[slot].name[sizeof(s_slots[slot].name) - 1] = '\0';
    s_slots[slot].dump = *dump;
    return 0;
}

int card_store_load(uint8_t slot, char *name, size_t name_len,
                    struct mfc_dump *dump)
{
    if (slot >= CARD_STORE_MAX_SLOTS || !s_slots[slot].occupied) return -1;
    if (name && name_len) {
        strncpy(name, s_slots[slot].name, name_len - 1);
        name[name_len - 1] = '\0';
    }
    if (dump) *dump = s_slots[slot].dump;
    return 0;
}

int card_store_delete(uint8_t slot)
{
    if (slot >= CARD_STORE_MAX_SLOTS) return -1;
    s_slots[slot].occupied = false;
    return 0;
}

int card_store_list(void (*print_fn)(uint8_t slot, const char *name))
{
    int n = 0;
    for (uint8_t i = 0; i < CARD_STORE_MAX_SLOTS; i++) {
        if (s_slots[i].occupied) {
            if (print_fn) print_fn(i, s_slots[i].name);
            n++;
        }
    }
    return n;
}

int card_store_count(void)
{
    int n = 0;
    for (uint8_t i = 0; i < CARD_STORE_MAX_SLOTS; i++) {
        if (s_slots[i].occupied) n++;
    }
    return n;
}

bool card_store_has(uint8_t slot)
{
    return slot < CARD_STORE_MAX_SLOTS && s_slots[slot].occupied;
}
