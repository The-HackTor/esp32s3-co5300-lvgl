#include "hw_sleep.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "hw_ir.h"
#include "smart_flipper/smart_flipper.h"
#include "store/ir_store.h"

#include <stdatomic.h>

static const char *TAG = "hw_sleep";

#define BOOT_BTN_GPIO  GPIO_NUM_0
#define IDLE_TICK_MS   1000
#define BTN_POLL_MS    50

static lv_display_t *s_disp;
static uint32_t      s_threshold_ms = 60u * 1000u;
static atomic_int    s_inhibits;
static lv_timer_t   *s_idle_timer;
static lv_timer_t   *s_btn_timer;
static bool          s_btn_prev_low;
static bool          s_btn_press_pending;

static void enter_light_sleep(void)
{
    ESP_LOGI(TAG, "light-sleep entry");

    app_panel_blank();
    hw_ir_tx_cancel_all(200);
    ir_history_flush();

    esp_light_sleep_start();

    s_btn_prev_low      = true;
    s_btn_press_pending = false;

    app_panel_restore_full();
    if(s_disp) lv_display_trigger_activity(s_disp);

    ESP_LOGI(TAG, "light-sleep wake (cause=%d)",
             (int)esp_sleep_get_wakeup_cause());
}

static void idle_tick(lv_timer_t *t)
{
    (void)t;
    if(!s_disp || s_threshold_ms == 0)         return;
    if(atomic_load(&s_inhibits) > 0)           return;
    if(lv_display_get_inactive_time(s_disp) < s_threshold_ms) return;
    enter_light_sleep();
}

static void btn_tick(lv_timer_t *t)
{
    (void)t;
    bool now_low = (gpio_get_level(BOOT_BTN_GPIO) == 0);
    bool was_low = s_btn_prev_low;
    s_btn_prev_low = now_low;

    if(now_low && !was_low) {
        s_btn_press_pending = true;
        return;
    }
    if(was_low && !now_low && s_btn_press_pending) {
        s_btn_press_pending = false;
        if(atomic_load(&s_inhibits) > 0) return;
        enter_light_sleep();
    }
}

void hw_sleep_init(lv_display_t *disp)
{
    s_disp = disp;

    const gpio_config_t boot_cfg = {
        .pin_bit_mask = 1ULL << BOOT_BTN_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&boot_cfg);

    gpio_wakeup_enable(BOOT_BTN_GPIO, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    s_btn_prev_low = (gpio_get_level(BOOT_BTN_GPIO) == 0);

    s_idle_timer = lv_timer_create(idle_tick, IDLE_TICK_MS, NULL);
    s_btn_timer  = lv_timer_create(btn_tick,  BTN_POLL_MS,  NULL);
    ESP_LOGI(TAG, "init: light-sleep threshold=%ums", (unsigned)s_threshold_ms);
}

void hw_sleep_set_threshold(uint32_t threshold_ms)
{
    s_threshold_ms = threshold_ms;
}

void hw_sleep_inhibit(bool inhibit)
{
    if(inhibit) {
        atomic_fetch_add(&s_inhibits, 1);
        return;
    }
    int prev = atomic_load(&s_inhibits);
    while(prev > 0) {
        if(atomic_compare_exchange_weak(&s_inhibits, &prev, prev - 1)) return;
    }
}
