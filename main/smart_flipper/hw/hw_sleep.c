#include "hw_sleep.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/atomic.h"

#include "hw_bat.h"
#include "touch_bsp.h"

#include <stdatomic.h>

static const char *TAG = "hw_sleep";

#define IR_RX_GPIO         GPIO_NUM_17
#define RTC_INT_GPIO       GPIO_NUM_15
#define IMU_INT_GPIO       GPIO_NUM_8

#define POLL_INTERVAL_US   (50 * 1000)         /* 50 ms touch poll */
#define IDLE_TICK_MS       1000

static lv_display_t       *s_disp;
static esp_timer_handle_t  s_lvgl_tick_timer;
static uint32_t            s_threshold_ms = 5 * 60 * 1000;

static atomic_int          s_inhibit_count;
static atomic_bool         s_napping;

static bool inhibited(void) { return atomic_load(&s_inhibit_count) > 0; }

/* Map every wake source we'll register; all are active-low, all hooked
 * via GPIO wakeup (light sleep) since they're on RTC-incapable IOs as
 * well as RTC-capable ones, and gpio_wakeup_enable handles both. */
static void arm_wake_sources(void)
{
    gpio_wakeup_enable(IR_RX_GPIO,   GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(RTC_INT_GPIO, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(IMU_INT_GPIO, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_sleep_enable_timer_wakeup(POLL_INTERVAL_US);
}

static void disarm_wake_sources(void)
{
    gpio_wakeup_disable(IR_RX_GPIO);
    gpio_wakeup_disable(RTC_INT_GPIO);
    gpio_wakeup_disable(IMU_INT_GPIO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
}

static bool wake_was_real(void)
{
    /* "Real" = anything other than the timer poll: a button-style wake
     * the user expects us to come fully back for. The timer wake is the
     * polling cycle that just checks for touch and goes back to sleep. */
    uint32_t causes = esp_sleep_get_wakeup_causes();
    const uint32_t HW_WAKE = (1u << ESP_SLEEP_WAKEUP_GPIO) |
                             (1u << ESP_SLEEP_WAKEUP_EXT0) |
                             (1u << ESP_SLEEP_WAKEUP_EXT1);
    if(causes & HW_WAKE) return true;
    if(causes & (1u << ESP_SLEEP_WAKEUP_TIMER)) {
        /* Touch poll: a single cheap I2C transaction. The driver
         * returns 0 when no finger is present. */
        uint16_t x = 0, y = 0;
        if(getTouch(&x, &y)) return true;
    }
    return false;
}

static void enter_light_sleep_loop(void)
{
    /* Pause the 2 ms LVGL tick timer so lv_tick_inc doesn't fire while
     * we're clock-gated; on resume LVGL just sees its tick frozen for
     * the sleep duration. Animations naturally pause and pick up where
     * they left off. */
    if(s_lvgl_tick_timer) esp_timer_stop(s_lvgl_tick_timer);

    arm_wake_sources();
    atomic_store(&s_napping, true);
    ESP_LOGI(TAG, "entering light-sleep loop");

    for(;;) {
        if(inhibited() || hw_bat_is_charging()) break;
        esp_light_sleep_start();
        if(wake_was_real()) break;
        /* Re-evaluate inhibit/charging on every wake -- the user might
         * have plugged in or a scene might have set the inhibit while
         * we were asleep. */
    }

    atomic_store(&s_napping, false);
    disarm_wake_sources();
    if(s_lvgl_tick_timer) esp_timer_start_periodic(s_lvgl_tick_timer, 2000);
    ESP_LOGI(TAG, "exiting light-sleep loop (causes=0x%lX)",
             (unsigned long)esp_sleep_get_wakeup_causes());
}

static void sleep_task(void *arg)
{
    (void)arg;
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(IDLE_TICK_MS));
        if(!s_disp || s_threshold_ms == 0) continue;
        if(inhibited())                    continue;
        if(hw_bat_is_charging())           continue;

        uint32_t inactive = lv_display_get_inactive_time(s_disp);
        if(inactive < s_threshold_ms)      continue;

        enter_light_sleep_loop();
    }
}

void hw_sleep_init(lv_display_t *disp, esp_timer_handle_t lvgl_tick_timer)
{
    s_disp            = disp;
    s_lvgl_tick_timer = lvgl_tick_timer;

    /* RTC_INT and IMU_INT pins are open-drain with external pulls; the
     * GPIO wakeup path needs them configured as plain inputs. IR_RX
     * (GPIO17) is already configured by hw_ir_init. */
    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << RTC_INT_GPIO) | (1ULL << IMU_INT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    xTaskCreate(sleep_task, "hw_sleep", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    ESP_LOGI(TAG, "init: light-sleep threshold %u ms", (unsigned)s_threshold_ms);
}

void hw_sleep_set_threshold(uint32_t threshold_ms)
{
    s_threshold_ms = threshold_ms;
}

void hw_sleep_inhibit(bool inhibit)
{
    if(inhibit) atomic_fetch_add(&s_inhibit_count, 1);
    else        atomic_fetch_sub(&s_inhibit_count, 1);
}

bool hw_sleep_is_napping(void)
{
    return atomic_load(&s_napping);
}
