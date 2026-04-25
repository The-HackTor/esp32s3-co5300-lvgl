#ifndef CARD_STORE_H
#define CARD_STORE_H

#include <nfc.h>

#define CARD_STORE_MAX_SLOTS 32

int card_store_init(void);
int card_store_save(uint8_t slot, const char *name, const struct mfc_dump *dump);
int card_store_load(uint8_t slot, char *name, size_t name_len,
		    struct mfc_dump *dump);
int card_store_delete(uint8_t slot);
int card_store_list(void (*print_fn)(uint8_t slot, const char *name));
int card_store_count(void);
bool card_store_has(uint8_t slot);

#endif
