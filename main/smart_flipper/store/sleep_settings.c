#include "sleep_settings.h"

#include "esp_log.h"
#include "hw/hw_bat.h"
#include "hw/hw_sleep.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "sleep_settings";

static SleepSettings s = {
    .dim_threshold_s          = 30,
    .blank_threshold_s        = 90,
    .light_sleep_threshold_s  = 300,
    .deep_sleep_threshold_s   = 1800,
    .listen_for_ir_when_asleep = true,
    .low_bat_aggressive       = true,
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
        if(sscanf(line, "dim_s=%u",    &v) == 1) s.dim_threshold_s         = v;
        if(sscanf(line, "blank_s=%u",  &v) == 1) s.blank_threshold_s       = v;
        if(sscanf(line, "light_s=%u",  &v) == 1) s.light_sleep_threshold_s = v;
        if(sscanf(line, "deep_s=%u",   &v) == 1) s.deep_sleep_threshold_s  = v;
        if(sscanf(line, "ir_listen=%u",&v) == 1) s.listen_for_ir_when_asleep = (v != 0);
        if(sscanf(line, "lowbat=%u",   &v) == 1) s.low_bat_aggressive      = (v != 0);
    }
    fclose(fp);
    return ESP_OK;
}

esp_err_t sleep_settings_save(void)
{
    FILE *fp = fopen(SLEEP_SETTINGS_PATH, "w");
    if(!fp) return ESP_FAIL;
    fprintf(fp, "dim_s=%u\n",     (unsigned)s.dim_threshold_s);
    fprintf(fp, "blank_s=%u\n",   (unsigned)s.blank_threshold_s);
    fprintf(fp, "light_s=%u\n",   (unsigned)s.light_sleep_threshold_s);
    fprintf(fp, "deep_s=%u\n",    (unsigned)s.deep_sleep_threshold_s);
    fprintf(fp, "ir_listen=%u\n", s.listen_for_ir_when_asleep ? 1u : 0u);
    fprintf(fp, "lowbat=%u\n",    s.low_bat_aggressive       ? 1u : 0u);
    fclose(fp);
    return ESP_OK;
}

void sleep_settings_set_dim_s(uint32_t v)      { s.dim_threshold_s = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_blank_s(uint32_t v)    { s.blank_threshold_s = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_light_s(uint32_t v)    { s.light_sleep_threshold_s = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_deep_s(uint32_t v)     { s.deep_sleep_threshold_s = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_ir_listen(bool v)      { s.listen_for_ir_when_asleep = v; sleep_settings_save(); sleep_settings_apply(); }
void sleep_settings_set_lowbat_agg(bool v)     { s.low_bat_aggressive = v; sleep_settings_save(); sleep_settings_apply(); }

void sleep_settings_apply(void)
{
    /* Low-battery override: when SoC drops below 15 %, ratchet the
     * thresholds down to extend run time at the cost of UI snappiness.
     * Charging detection short-circuits this -- if the user is plugged
     * in we honor the configured values. */
    uint32_t light_ms = s.light_sleep_threshold_s * 1000u;
    uint32_t deep_ms  = s.deep_sleep_threshold_s  * 1000u;

    if(s.low_bat_aggressive && !hw_bat_is_charging() && hw_bat_read_soc_pct() < 15) {
        if(light_ms > 60u   * 1000u) light_ms = 60u   * 1000u;     /* 1 min   */
        if(deep_ms  > 300u  * 1000u) deep_ms  = 300u  * 1000u;     /* 5 min   */
    }

    hw_sleep_set_threshold(light_ms);
    hw_sleep_set_deep_threshold(deep_ms);
}
