#include "smart_flipper.h"
#include "esp_log.h"
#include "lvgl.h"

#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/styles.h"
#include "ui/ui_subjects.h"
#include "ui/arc_menu.h"
#include "ui/screens/scr_watchface.h"
#include "ui/screens/scr_weather.h"
#include "ui/screens/scr_music.h"
#include "ui/screens/scr_notifications.h"
#include "ui/screens/scr_settings.h"
#include "hw/hw_nfc.h"
#include "hw/hw_subghz.h"
#include "store/card_store.h"
#include "store/signal_store.h"
#include "bridge/bridge.h"

static const char *TAG = "smart_flipper";

void nfc_app_register(void);
void subghz_app_register(void);
void ir_app_register(void);

static void clock_tick_cb(lv_timer_t *t)
{
    (void)t;
    int s = lv_subject_get_int(&subject_second) + 1;
    int m = lv_subject_get_int(&subject_minute);
    int h = lv_subject_get_int(&subject_hour);

    if (s >= 60) { s = 0; m++; }
    if (m >= 60) { m = 0; h++; }
    if (h >= 24) { h = 0; }

    lv_subject_set_int(&subject_second, s);
    lv_subject_set_int(&subject_minute, m);
    lv_subject_set_int(&subject_hour, h);

    static char buf[8];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    lv_subject_copy_string(&subject_time, buf);
}

void smart_flipper_start(void)
{
    ESP_LOGI(TAG, "smart_flipper_start: init stores + UI");

    card_store_init();
    signal_store_init();

    styles_init();
    ui_subjects_init();

    hw_nfc_init(false);
    hw_subghz_init(NULL, false);

    lv_obj_t *watchface = scr_watchface_create();
    lv_screen_load(watchface);

    app_manager_init(watchface);
    arc_menu_init();

    scr_weather_register();
    scr_music_register();
    scr_notifications_register();
    scr_settings_register();
    nfc_app_register();
    subghz_app_register();
    ir_app_register();
    app_manager_init_apps();

    bridge_init();

    lv_timer_create(clock_tick_cb, 1000, NULL);

    ESP_LOGI(TAG, "smart_flipper_start: done, watchface loaded");
}
