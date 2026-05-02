#include "hw_sleep.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "hw_ir.h"
#include "smart_flipper/smart_flipper.h"
#include "store/ir_store.h"

#include <stdatomic.h>

static const char *TAG = "hw_sleep";

#define BOOT_BTN_GPIO  GPIO_NUM_0
#define IDLE_TICK_MS   1000

static lv_display_t *s_disp;
static uint32_t      s_threshold_ms = 60u * 1000u;
static atomic_int    s_inhibits;
static lv_timer_t   *s_idle_timer;

static void IRAM_ATTR wake_isr(void *arg) { (void)arg; }

static void enter_light_sleep(void)
{
    ESP_LOGI(TAG, "light-sleep entry");

    hw_ir_tx_cancel_all(200);
    ir_history_flush();
    app_panel_blank();

    esp_light_sleep_start();

    app_panel_restore_full();
    if(s_disp) lv_display_trigger_activity(s_disp);

    ESP_LOGI(TAG, "light-sleep wake (cause=0x%lx)",
             (unsigned long)esp_sleep_get_wakeup_causes());
}

static void idle_tick(lv_timer_t *t)
{
    (void)t;
    if(!s_disp || s_threshold_ms == 0)         return;
    if(atomic_load(&s_inhibits) > 0)           return;
    if(lv_display_get_inactive_time(s_disp) < s_threshold_ms) return;
    enter_light_sleep();
}

void hw_sleep_init(lv_display_t *disp)
{
    s_disp = disp;

    const gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOOT_BTN_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_LOW_LEVEL,
    };
    gpio_config(&cfg);

    esp_err_t isr_err = gpio_install_isr_service(0);
    if(isr_err != ESP_OK && isr_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(isr_err));
    }
    gpio_isr_handler_add(BOOT_BTN_GPIO, wake_isr, NULL);

    gpio_wakeup_enable(BOOT_BTN_GPIO, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    s_idle_timer = lv_timer_create(idle_tick, IDLE_TICK_MS, NULL);
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
