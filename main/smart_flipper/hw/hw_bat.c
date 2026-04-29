#include "hw_bat.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "hw_bat";

#define BAT_ADC_UNIT       ADC_UNIT_1
#define BAT_ADC_CHAN       ADC_CHANNEL_3        /* IO4 */
#define BAT_ADC_ATTEN      ADC_ATTEN_DB_12      /* ~150..3100 mV input range */
#define BAT_DIVIDER_NUM    3                    /* 200K + 100K -> /3 at the tap */
#define BAT_FILTER_DEPTH   8

/* Charging detection. Trend is computed over the last
 * CHARGING_TREND_WINDOW_S seconds; >= +CHARGING_TREND_THRESHOLD_MV in
 * that window is "rising" (USB plugged in). The plateau check covers
 * the case where battery is already topped off and trend is flat. */
#define CHARGING_PLATEAU_MV       4150
#define CHARGING_TREND_WINDOW_S   3
#define CHARGING_TREND_THRESHOLD  15            /* mV over the window */

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t          s_cali_handle;
static bool                       s_inited;
static bool                       s_have_cali;
static int                        s_filter_buf[BAT_FILTER_DEPTH];
static size_t                     s_filter_idx;
static size_t                     s_filter_count;
static int                        s_last_mv;

static int                        s_trend_buf[CHARGING_TREND_WINDOW_S + 1];
static int64_t                    s_last_trend_us;

static int read_raw_mv(void)
{
    if(!s_inited) return 0;

    int raw = 0;
    if(adc_oneshot_read(s_adc_handle, BAT_ADC_CHAN, &raw) != ESP_OK) return 0;

    int adc_mv = 0;
    if(s_have_cali) {
        if(adc_cali_raw_to_voltage(s_cali_handle, raw, &adc_mv) != ESP_OK) {
            /* Calibration miss -- fall through to linear estimate. */
            adc_mv = (raw * 3100) / 4095;
        }
    } else {
        adc_mv = (raw * 3100) / 4095;
    }
    return adc_mv * BAT_DIVIDER_NUM;
}

static void filter_push(int sample_mv)
{
    s_filter_buf[s_filter_idx] = sample_mv;
    s_filter_idx = (s_filter_idx + 1) % BAT_FILTER_DEPTH;
    if(s_filter_count < BAT_FILTER_DEPTH) s_filter_count++;

    int64_t sum = 0;
    for(size_t i = 0; i < s_filter_count; i++) sum += s_filter_buf[i];
    s_last_mv = (int)(sum / (int64_t)s_filter_count);
}

static void trend_tick(int sample_mv)
{
    /* Push one bucket per second; trend slot count = CHARGING_TREND_WINDOW_S+1
     * so we have window-start and window-end samples. */
    int64_t now_us = esp_timer_get_time();
    if(s_last_trend_us == 0 ||
       (now_us - s_last_trend_us) >= (int64_t)1000000) {
        for(int i = CHARGING_TREND_WINDOW_S; i > 0; i--) {
            s_trend_buf[i] = s_trend_buf[i - 1];
        }
        s_trend_buf[0] = sample_mv;
        s_last_trend_us = now_us;
    }
}

static void bat_task(void *arg)
{
    (void)arg;
    /* Sample every 250 ms. With 8-deep filter that gives a 2-second
     * smoothing constant, plenty fast for a status-bar glyph and slow
     * enough to cancel transient TX-load brownouts on the rail. */
    for(;;) {
        int mv = read_raw_mv();
        if(mv > 0) {
            filter_push(mv);
            trend_tick(s_last_mv);
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void hw_bat_init(void)
{
    if(s_inited) return;

    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BAT_ADC_UNIT,
    };
    if(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle) != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed");
        return;
    }

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = BAT_ADC_ATTEN,
    };
    if(adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHAN, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed");
        return;
    }

    /* Curve-fitting calibration is the preferred scheme on S3; fall
     * back to the line-fitting path on chips where curve isn't fused. */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = BAT_ADC_UNIT,
        .atten    = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if(adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle) == ESP_OK) {
        s_have_cali = true;
    }
#endif

    s_inited = true;
    ESP_LOGI(TAG, "init: IO4 ADC1_CH3, atten 12 dB, divider 1/3, cali=%d",
             (int)s_have_cali);

    xTaskCreate(bat_task, "hw_bat", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
}

int hw_bat_read_mv(void)
{
    return s_last_mv;
}

int hw_bat_read_soc_pct(void)
{
    /* 1S LiPo discharge curve (typical, light load). Linear interpolation
     * between knot voltages; saturates at the endpoints. The flat
     * 3.7-3.8 V plateau is where most of the battery's energy lives, so
     * a coarse table is fine -- a 50 mV miss is well under the noise. */
    static const struct { int mv; int pct; } curve[] = {
        { 4200, 100 },
        { 4100,  90 },
        { 4000,  80 },
        { 3900,  70 },
        { 3800,  60 },
        { 3700,  50 },
        { 3650,  40 },
        { 3600,  30 },
        { 3550,  20 },
        { 3500,  10 },
        { 3400,   5 },
        { 3300,   0 },
    };
    const int mv = s_last_mv;
    if(mv >= curve[0].mv) return 100;
    const size_t n = sizeof(curve) / sizeof(curve[0]);
    if(mv <= curve[n - 1].mv) return 0;
    for(size_t i = 0; i + 1 < n; i++) {
        if(mv <= curve[i].mv && mv >= curve[i + 1].mv) {
            int dv  = curve[i].mv  - curve[i + 1].mv;
            int dp  = curve[i].pct - curve[i + 1].pct;
            int off = curve[i].mv  - mv;
            return curve[i].pct - (dp * off) / dv;
        }
    }
    return 0;
}

bool hw_bat_is_charging(void)
{
    if(s_last_mv >= CHARGING_PLATEAU_MV) return true;
    if(s_filter_count < BAT_FILTER_DEPTH) return false;

    /* Trend window: oldest valid sample to newest. */
    int newest = s_trend_buf[0];
    int oldest = s_trend_buf[CHARGING_TREND_WINDOW_S];
    if(oldest == 0) return false;        /* not yet populated */
    return (newest - oldest) >= CHARGING_TREND_THRESHOLD;
}
