#ifndef IR_UNIVERSAL_INDEX_H
#define IR_UNIVERSAL_INDEX_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "store/ir_store.h"

/*
 * Reverse lookup from a captured (protocol, address) onto the embedded
 * universal DB. On a hit, callers can offer the user a "Looks like a TV
 * remote -- open Universal TVs?" shortcut into ir_SCENE_UniversalCategory.
 *
 * Build the index once after ir_universal_db_init has populated the
 * embedded category remotes.
 */
esp_err_t ir_universal_index_init(void);

typedef enum {
    IR_MATCH_NONE  = 0,
    IR_MATCH_GROUP = 1,  /* protocol + address match (related buttons on same remote group) */
    IR_MATCH_EXACT = 2,  /* protocol + address + command match (this exact button is in the DB) */
} IrMatchConfidence;

bool      ir_universal_index_match(const char *protocol, uint32_t address,
                                   uint32_t command,
                                   IrUniversalCategory *out_cat,
                                   char *out_label, size_t out_label_len,
                                   IrMatchConfidence *out_confidence,
                                   uint16_t *out_group_size);

#endif
