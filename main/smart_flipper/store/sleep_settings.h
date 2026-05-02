#ifndef SLEEP_SETTINGS_H
#define SLEEP_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SLEEP_SETTINGS_PATH "/sdcard/sleep_v2.ini"

typedef struct {
    uint32_t dim_threshold_s;
    uint32_t blank_threshold_s;
    uint32_t light_sleep_threshold_s;
    bool     low_bat_aggressive;
} SleepSettings;

const SleepSettings *sleep_settings(void);
esp_err_t            sleep_settings_load(void);
esp_err_t            sleep_settings_save(void);

void sleep_settings_set_dim_s     (uint32_t s);
void sleep_settings_set_blank_s   (uint32_t s);
void sleep_settings_set_light_s    (uint32_t s);
void sleep_settings_set_lowbat_agg(bool v);

void sleep_settings_apply(void);

#endif
