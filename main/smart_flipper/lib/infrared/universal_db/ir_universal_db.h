#ifndef IR_UNIVERSAL_DB_H
#define IR_UNIVERSAL_DB_H

#include "esp_err.h"
#include "store/ir_store.h"

esp_err_t       ir_universal_db_init(void);

size_t          ir_universal_db_button_count(IrUniversalCategory cat);
const char     *ir_universal_db_button_name(IrUniversalCategory cat, size_t idx);
size_t          ir_universal_db_signal_count(IrUniversalCategory cat, size_t button_idx);
const IrButton *ir_universal_db_button_signal(IrUniversalCategory cat,
                                              size_t button_idx,
                                              size_t signal_idx);

#endif
