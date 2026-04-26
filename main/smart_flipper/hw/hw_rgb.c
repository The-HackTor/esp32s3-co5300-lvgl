#include "hw_rgb.h"

#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "hw_rgb";

#define RGB_GPIO_R       3
#define RGB_GPIO_G       2
#define RGB_GPIO_B       1

#define RGB_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define RGB_LEDC_TIMER   LEDC_TIMER_0
#define RGB_LEDC_RES     LEDC_TIMER_13_BIT
#define RGB_LEDC_FREQ_HZ 5000

#define RGB_CH_R         LEDC_CHANNEL_0
#define RGB_CH_G         LEDC_CHANNEL_1
#define RGB_CH_B         LEDC_CHANNEL_2

#define RGB_DUTY_FULL    ((1U << 13) - 1U)         /* 8191 */
#define RGB_DUTY_CAP_PCT 15
#define RGB_DUTY_CAP     (RGB_DUTY_FULL * RGB_DUTY_CAP_PCT / 100)

static esp_timer_handle_t s_pulse_timer;
static bool               s_inited;

static uint32_t scale_to_duty(uint8_t v)
{
    /* 0..255 -> 0..RGB_DUTY_CAP, linear */
    return (uint32_t)v * RGB_DUTY_CAP / 255U;
}

static void apply_channel(ledc_channel_t ch, uint8_t v)
{
    ledc_set_duty(RGB_LEDC_MODE, ch, scale_to_duty(v));
    ledc_update_duty(RGB_LEDC_MODE, ch);
}

static void pulse_timer_cb(void *arg)
{
    (void)arg;
    apply_channel(RGB_CH_R, 0);
    apply_channel(RGB_CH_G, 0);
    apply_channel(RGB_CH_B, 0);
}

static void install_channel(ledc_channel_t ch, int gpio)
{
    const ledc_channel_config_t cfg = {
        .speed_mode     = RGB_LEDC_MODE,
        .channel        = ch,
        .timer_sel      = RGB_LEDC_TIMER,
        .gpio_num       = gpio,
        .duty           = 0,
        .hpoint         = 0,
        .flags.output_invert = 1,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&cfg));
}

void hw_rgb_init(void)
{
    if(s_inited) return;

    const ledc_timer_config_t timer = {
        .speed_mode      = RGB_LEDC_MODE,
        .timer_num       = RGB_LEDC_TIMER,
        .duty_resolution = RGB_LEDC_RES,
        .freq_hz         = RGB_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    install_channel(RGB_CH_R, RGB_GPIO_R);
    install_channel(RGB_CH_G, RGB_GPIO_G);
    install_channel(RGB_CH_B, RGB_GPIO_B);

    const esp_timer_create_args_t args = {
        .callback = pulse_timer_cb,
        .name     = "rgb_pulse",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_pulse_timer));

    s_inited = true;
    ESP_LOGI(TAG, "init: R=GPIO%d G=GPIO%d B=GPIO%d, %d-bit @ %d Hz, cap=%d%%",
             RGB_GPIO_R, RGB_GPIO_G, RGB_GPIO_B,
             (int)RGB_LEDC_RES, RGB_LEDC_FREQ_HZ, RGB_DUTY_CAP_PCT);
}

void hw_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    if(!s_inited) return;
    esp_timer_stop(s_pulse_timer);
    apply_channel(RGB_CH_R, r);
    apply_channel(RGB_CH_G, g);
    apply_channel(RGB_CH_B, b);
}

void hw_rgb_off(void)
{
    hw_rgb_set(0, 0, 0);
}

void hw_rgb_pulse(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms)
{
    if(!s_inited) return;
    esp_timer_stop(s_pulse_timer);
    apply_channel(RGB_CH_R, r);
    apply_channel(RGB_CH_G, g);
    apply_channel(RGB_CH_B, b);
    esp_timer_start_once(s_pulse_timer, (uint64_t)duration_ms * 1000ULL);
}
