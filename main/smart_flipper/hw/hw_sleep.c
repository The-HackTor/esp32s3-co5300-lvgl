#include "hw_sleep.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/atomic.h"

#include "hw_bat.h"
#include "touch_bsp.h"

#include <stdatomic.h>
#include <string.h>

static const char *TAG = "hw_sleep";

#define IR_RX_GPIO         GPIO_NUM_17
#define RTC_INT_GPIO       GPIO_NUM_15
#define IMU_INT_GPIO       GPIO_NUM_8

#define POLL_INTERVAL_US   (50 * 1000)         /* 50 ms touch poll */
#define IDLE_TICK_MS       1000

static lv_display_t       *s_disp;
static esp_timer_handle_t  s_lvgl_tick_timer;
static uint32_t            s_threshold_ms      = 5 * 60 * 1000;
static uint32_t            s_deep_threshold_ms = 30 * 60 * 1000;

static atomic_int          s_inhibit_count;
static atomic_bool         s_napping;

/* RTC slow memory survives deep sleep (not power-off). The magic word
 * lets the post-wake boot code distinguish a clean cold boot (BSS-zero
 * payload) from a warm wake with a saved remote path. */
#define RESUME_MAGIC  0x5AFE517Eu

typedef struct {
    uint32_t magic;
    char     payload[96];
} resume_slot_t;

RTC_DATA_ATTR static resume_slot_t s_resume;
static bool                        s_woke_from_deep;

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

/* Save RTC-domain state and call esp_deep_sleep_start. Never returns;
 * wake re-runs app_main from the top with the resume slot intact. */
static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "entering deep sleep (resume payload='%s')",
             s_resume.magic == RESUME_MAGIC ? s_resume.payload : "");

    /* Wake sources for deep sleep: ext1 (any LOW) on the IR / RTC /
     * IMU lines so a remote button press, an RTC alarm match, or a
     * shake brings us back. ESP32-S3 deep sleep also accepts a timer
     * wakeup so we periodically check in even with no external event;
     * default 1 hour. */
    const uint64_t ext1_mask =
        (1ULL << 17) |  /* IR_RX  */
        (1ULL << 15) |  /* RTC_INT */
        (1ULL <<  8);   /* IMU_INT */

    esp_sleep_enable_ext1_wakeup_io(ext1_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_sleep_enable_timer_wakeup(60ULL * 60ULL * 1000000ULL);

    esp_deep_sleep_start();
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

    int64_t loop_start_us = esp_timer_get_time();

    for(;;) {
        if(inhibited() || hw_bat_is_charging()) break;
        esp_light_sleep_start();
        if(wake_was_real()) break;

        /* Escalate to deep sleep after the configured run-up. We've
         * spent N polling cycles in light-sleep without a real wake;
         * the user is gone, drop to ~50 uA-1 mA. */
        if(s_deep_threshold_ms != 0) {
            int64_t elapsed_ms = (esp_timer_get_time() - loop_start_us) / 1000;
            if(elapsed_ms >= (int64_t)s_deep_threshold_ms) {
                disarm_wake_sources();
                /* Tick timer stays stopped; deep sleep doesn't return. */
                enter_deep_sleep();
                /* unreachable */
            }
        }
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

    /* Latch the wake reason at boot. After the first call the cause is
     * lost (esp_sleep_get_wakeup_causes returns UNDEFINED), so the
     * resume payload getter checks the captured flag instead. */
    {
        const uint32_t HW_WAKE_CAUSES =
            (1u << ESP_SLEEP_WAKEUP_GPIO) |
            (1u << ESP_SLEEP_WAKEUP_EXT0) |
            (1u << ESP_SLEEP_WAKEUP_EXT1) |
            (1u << ESP_SLEEP_WAKEUP_TIMER);
        s_woke_from_deep = (esp_sleep_get_wakeup_causes() & HW_WAKE_CAUSES) != 0;
        if(!s_woke_from_deep) {
            /* Cold boot -- invalidate any stale resume slot the linker
             * may have populated from RTC slow memory contents. */
            s_resume.magic = 0;
            s_resume.payload[0] = '\0';
        }
    }

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

void hw_sleep_set_deep_threshold(uint32_t threshold_ms)
{
    s_deep_threshold_ms = threshold_ms;
}

void hw_sleep_set_resume_payload(const char *path)
{
    if(!path || path[0] == '\0') {
        s_resume.magic = 0;
        s_resume.payload[0] = '\0';
        return;
    }
    s_resume.magic = RESUME_MAGIC;
    strncpy(s_resume.payload, path, sizeof(s_resume.payload) - 1);
    s_resume.payload[sizeof(s_resume.payload) - 1] = '\0';
}

const char *hw_sleep_get_resume_payload(void)
{
    if(!s_woke_from_deep)             return NULL;
    if(s_resume.magic != RESUME_MAGIC) return NULL;
    return s_resume.payload;
}

bool hw_sleep_woke_from_deep(void)
{
    return s_woke_from_deep;
}
