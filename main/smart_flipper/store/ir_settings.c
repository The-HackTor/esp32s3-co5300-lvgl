#include "ir_settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IrSettings s = {
    .brute_gap_ms     = 150,
    .brute_ac_gap_ms  = 250,
    .tx_echo_ms       = 100,
    .history_max      = 64,
    .brute_repeat     = 3,
    .auto_save_worked = false,
    .tx_invert        = false,
};

const IrSettings *ir_settings(void) { return &s; }

static const char *skip_ws(const char *p)
{
    while(*p == ' ' || *p == '\t') p++;
    return p;
}

esp_err_t ir_settings_load(void)
{
    FILE *fp = fopen(IR_SETTINGS_PATH, "r");
    if(!fp) return ESP_OK;

    char line[128];
    while(fgets(line, sizeof(line), fp)) {
        char *eq = strchr(line, '=');
        if(!eq) continue;
        *eq = '\0';
        const char *k = skip_ws(line);
        const char *v = skip_ws(eq + 1);

        char key[32];
        size_t i = 0;
        while(*k && *k != ' ' && *k != '\t' && i + 1 < sizeof(key)) {
            key[i++] = *k++;
        }
        key[i] = '\0';

        long iv = atol(v);
        if(strcmp(key, "brute_gap_ms") == 0) {
            if(iv >= 50 && iv <= 1000) s.brute_gap_ms = (uint16_t)iv;
        } else if(strcmp(key, "brute_ac_gap_ms") == 0) {
            if(iv >= 100 && iv <= 2000) s.brute_ac_gap_ms = (uint16_t)iv;
        } else if(strcmp(key, "tx_echo_ms") == 0) {
            if(iv >= 0 && iv <= 1000) s.tx_echo_ms = (uint16_t)iv;
        } else if(strcmp(key, "history_max") == 0) {
            if(iv >= 8 && iv <= 256) s.history_max = (uint16_t)iv;
        } else if(strcmp(key, "brute_repeat") == 0) {
            if(iv >= 1 && iv <= 10) s.brute_repeat = (uint8_t)iv;
        } else if(strcmp(key, "auto_save") == 0) {
            s.auto_save_worked = (iv != 0);
        } else if(strcmp(key, "tx_invert") == 0) {
            s.tx_invert = (iv != 0);
        }
    }
    fclose(fp);
    return ESP_OK;
}

esp_err_t ir_settings_save(void)
{
    FILE *fp = fopen(IR_SETTINGS_PATH, "w");
    if(!fp) return ESP_FAIL;
    fprintf(fp, "[ir]\n");
    fprintf(fp, "brute_gap_ms=%u\n",    s.brute_gap_ms);
    fprintf(fp, "brute_ac_gap_ms=%u\n", s.brute_ac_gap_ms);
    fprintf(fp, "tx_echo_ms=%u\n",      s.tx_echo_ms);
    fprintf(fp, "history_max=%u\n",     s.history_max);
    fprintf(fp, "brute_repeat=%u\n",    s.brute_repeat);
    fprintf(fp, "auto_save=%d\n",       s.auto_save_worked ? 1 : 0);
    fprintf(fp, "tx_invert=%d\n",       s.tx_invert ? 1 : 0);
    fclose(fp);
    return ESP_OK;
}

void ir_settings_set_brute_gap_ms   (uint16_t v) { s.brute_gap_ms     = v; ir_settings_save(); }
void ir_settings_set_brute_ac_gap_ms(uint16_t v) { s.brute_ac_gap_ms  = v; ir_settings_save(); }
void ir_settings_set_tx_echo_ms     (uint16_t v) { s.tx_echo_ms       = v; ir_settings_save(); }
void ir_settings_set_history_max    (uint16_t v) { s.history_max      = v; ir_settings_save(); }
void ir_settings_set_brute_repeat   (uint8_t  v) { s.brute_repeat     = v; ir_settings_save(); }
void ir_settings_set_auto_save      (bool v)     { s.auto_save_worked = v; ir_settings_save(); }
void ir_settings_set_tx_invert      (bool v)     { s.tx_invert        = v; ir_settings_save(); }
