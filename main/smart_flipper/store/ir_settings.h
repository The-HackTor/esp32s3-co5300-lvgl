#ifndef IR_SETTINGS_H
#define IR_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define IR_SETTINGS_PATH "/sdcard/ir/settings.ini"

typedef struct {
    uint16_t brute_gap_ms;       /* default 150 */
    uint16_t brute_ac_gap_ms;    /* default 250 */
    uint16_t tx_echo_ms;         /* default 100 */
    uint16_t history_max;        /* default 64  */
    bool     auto_save_worked;   /* default false */
} IrSettings;

const IrSettings *ir_settings(void);
esp_err_t         ir_settings_load(void);
esp_err_t         ir_settings_save(void);

void ir_settings_set_brute_gap_ms   (uint16_t v);
void ir_settings_set_brute_ac_gap_ms(uint16_t v);
void ir_settings_set_tx_echo_ms     (uint16_t v);
void ir_settings_set_history_max    (uint16_t v);
void ir_settings_set_auto_save      (bool v);

#endif
