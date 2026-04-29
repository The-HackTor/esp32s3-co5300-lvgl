#ifndef SLEEP_SETTINGS_H
#define SLEEP_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SLEEP_SETTINGS_PATH "/sdcard/sleep.ini"

/* User-facing values are seconds; the engine multiplies internally. */
typedef struct {
    uint32_t dim_threshold_s;          /* default 30   */
    uint32_t blank_threshold_s;        /* default 90   */
    uint32_t light_sleep_threshold_s;  /* default 300  */
    uint32_t deep_sleep_threshold_s;   /* default 1800 */
    bool     listen_for_ir_when_asleep;/* default true */
    bool     low_bat_aggressive;       /* default true; SoC<15% drops thresholds */
} SleepSettings;

const SleepSettings *sleep_settings(void);
esp_err_t            sleep_settings_load(void);
esp_err_t            sleep_settings_save(void);

void sleep_settings_set_dim_s     (uint32_t s);
void sleep_settings_set_blank_s   (uint32_t s);
void sleep_settings_set_light_s   (uint32_t s);
void sleep_settings_set_deep_s    (uint32_t s);
void sleep_settings_set_ir_listen (bool v);
void sleep_settings_set_lowbat_agg(bool v);

/* Apply current values to hw_sleep (call after load and after each
 * mutation). The engine reads the deep / light thresholds; the dim /
 * blank thresholds are read from sleep_settings() by app_main's idle
 * tick callback directly. */
void sleep_settings_apply(void);

#endif
