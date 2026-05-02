#include "sleep_settings.h"

#include "esp_log.h"
#include "hw/hw_sleep.h"
#include "store_util.h"

#include <stdio.h>

static const char *TAG = "sleep_settings";

static SleepSettings s = {
    .dim_threshold_s        = 15,
    .blank_threshold_s      = 35,
    .light_sleep_threshold_s = 60,
    .low_bat_aggressive     = true,
};

const SleepSettings *sleep_settings(void) { return &s; }

esp_err_t sleep_settings_load(void)
{
    FILE *fp = fopen(SLEEP_SETTINGS_PATH, "r");
    if(!fp) {
        ESP_LOGI(TAG, "no settings file; using defaults");
        return ESP_ERR_NOT_FOUND;
    }
    char line[64];
    while(fgets(line, sizeof(line), fp)) {
        unsigned v;
        if(sscanf(line, "dim_s=%u",   &v) == 1) s.dim_threshold_s        = v;
        if(sscanf(line, "blank_s=%u", &v) == 1) s.blank_threshold_s      = v;
        if(sscanf(line, "light_s=%u",  &v) == 1) s.light_sleep_threshold_s = v;
        if(sscanf(line, "lowbat=%u",  &v) == 1) s.low_bat_aggressive     = (v != 0);
    }
    fclose(fp);
    return ESP_OK;
}

esp_err_t sleep_settings_save(void)
{
    FILE *fp = store_atomic_open(SLEEP_SETTINGS_PATH);
    if(!fp) return ESP_FAIL;
    if(fprintf(fp, "dim_s=%u\n",   (unsigned)s.dim_threshold_s)        < 0 ||
       fprintf(fp, "blank_s=%u\n", (unsigned)s.blank_threshold_s)      < 0 ||
       fprintf(fp, "light_s=%u\n",  (unsigned)s.light_sleep_threshold_s) < 0 ||
       fprintf(fp, "lowbat=%u\n",  s.low_bat_aggressive ? 1u : 0u)     < 0) {
        store_atomic_abort(fp, SLEEP_SETTINGS_PATH);
        return ESP_FAIL;
    }
    return store_atomic_commit(fp, SLEEP_SETTINGS_PATH);
}

void sleep_settings_set_dim_s(uint32_t v)      { s.dim_threshold_s = v;        sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_blank_s(uint32_t v)    { s.blank_threshold_s = v;      sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_light_s(uint32_t v)     { s.light_sleep_threshold_s = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_lowbat_agg(bool v)     { s.low_bat_aggressive = v;     sleep_settings_save(); sleep_settings_apply(); }

void sleep_settings_apply(void)
{
    hw_sleep_set_threshold(s.light_sleep_threshold_s * 1000u);
}
