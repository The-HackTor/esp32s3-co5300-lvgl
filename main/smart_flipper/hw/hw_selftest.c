#include "hw_selftest.h"
#include "hw_rgb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "hw_selftest"

#define PROBE_FREQ_HZ      38000
#define PROBE_DUTY_RES     LEDC_TIMER_10_BIT
#define PROBE_DUTY_MAX     ((1U << 10) - 1U)
#define PROBE_DUTY_33PCT   (PROBE_DUTY_MAX / 3U)
#define PROBE_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define PROBE_LEDC_TIMER   LEDC_TIMER_1
#define PROBE_LEDC_CHAN    LEDC_CHANNEL_5
#define PROBE_HOLD_MS      3000

/* Plausible IR_TX candidates: every GPIO exposed on the Waveshare J1/J3
 * expansion headers EXCEPT pins already in use (RGB 1/2/3, AMOLED 9..14/21/42,
 * SD 38..41, IMU/RTC/UART, USB U_N/U_P 19/20). */
static const int CANDIDATES[] = {
    5, 6, 7, 16, 17, 18, 45, 46, 47, 48,
};
#define N_CANDIDATES (sizeof(CANDIDATES)/sizeof(CANDIDATES[0]))

static void probe_pin(int gpio)
{
    ESP_LOGW(TAG, ">>> PROBE GPIO%d for %d ms <<<", gpio, PROBE_HOLD_MS);

    const ledc_timer_config_t tcfg = {
        .speed_mode      = PROBE_LEDC_MODE,
        .timer_num       = PROBE_LEDC_TIMER,
        .duty_resolution = PROBE_DUTY_RES,
        .freq_hz         = PROBE_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    if(ledc_timer_config(&tcfg) != ESP_OK) {
        ESP_LOGE(TAG, "  timer cfg failed for GPIO%d", gpio);
        return;
    }

    const ledc_channel_config_t ccfg = {
        .speed_mode = PROBE_LEDC_MODE,
        .channel    = PROBE_LEDC_CHAN,
        .timer_sel  = PROBE_LEDC_TIMER,
        .gpio_num   = gpio,
        .duty       = PROBE_DUTY_33PCT,
        .hpoint     = 0,
        .flags.output_invert = 0,
    };
    if(ledc_channel_config(&ccfg) != ESP_OK) {
        ESP_LOGE(TAG, "  channel cfg failed for GPIO%d", gpio);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(PROBE_HOLD_MS));

    ledc_stop(PROBE_LEDC_MODE, PROBE_LEDC_CHAN, 0);
    gpio_reset_pin((gpio_num_t)gpio);
    vTaskDelay(pdMS_TO_TICKS(300));
}

void hw_selftest_run(void)
{
    hw_rgb_off();
    ESP_LOGW(TAG, "BEGIN IR_TX GPIO discovery probe");
    ESP_LOGW(TAG, "Watch your phone camera; the IR LEDs glow on the right pin");
    ESP_LOGW(TAG, "Total: %d candidates x %d ms", (int)N_CANDIDATES, PROBE_HOLD_MS);

    for(size_t i = 0; i < N_CANDIDATES; i++) {
        probe_pin(CANDIDATES[i]);
    }

    ESP_LOGW(TAG, "END probe -- which GPIO lit the IR LEDs? Update IR_TX_GPIO.");
}
