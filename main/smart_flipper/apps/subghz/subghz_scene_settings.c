#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "hw/hw_subghz.h"

static const uint32_t freq_list[] = {
    300000, 303875, 304250, 310000, 315000, 318000,
    390000, 418000, 433075, 433420, 433920, 434420, 434775, 438900,
    868350, 915000, 925000,
};
#define FREQ_COUNT (sizeof(freq_list) / sizeof(freq_list[0]))

static const char *mod_labels[] = {"AM270", "AM650", "FM2.38", "FM47.6"};
#define MOD_COUNT 4

static const float rssi_list[] = {
    -120.0f, -85.0f, -80.0f, -75.0f, -70.0f,
    -65.0f, -60.0f, -55.0f, -50.0f, -45.0f, -40.0f,
};
static const char *rssi_labels[] = {
    "Off", "-85", "-80", "-75", "-70",
    "-65", "-60", "-55", "-50", "-45", "-40",
};
#define RSSI_COUNT (sizeof(rssi_list) / sizeof(rssi_list[0]))

enum {
    CFG_FREQUENCY,
    CFG_HOPPING,
    CFG_MODULATION,
    CFG_RSSI,
};

static int find_freq_idx(uint32_t freq)
{
    for(int i = 0; i < (int)FREQ_COUNT; i++) {
        if(freq_list[i] == freq) return i;
    }
    return 8;
}

static int find_rssi_idx(float val)
{
    for(int i = 0; i < (int)RSSI_COUNT; i++) {
        if(rssi_list[i] >= val - 0.5f && rssi_list[i] <= val + 0.5f) return i;
    }
    return 0;
}

static void submenu_cb(void *context, uint32_t index)
{
    SubghzApp *app = context;

    switch(index) {
    case CFG_FREQUENCY:
        if(!app->hopping) {
            int idx = (find_freq_idx(app->frequency) + 1) % FREQ_COUNT;
            app->frequency = freq_list[idx];
            hw_subghz_set_frequency(app->frequency);
        }
        break;
    case CFG_HOPPING:
        app->hopping = !app->hopping;
        break;
    case CFG_MODULATION:
        app->preset = (app->preset + 1) % MOD_COUNT;
        hw_subghz_set_preset(app->preset);
        break;
    case CFG_RSSI: {
        int idx = (find_rssi_idx(app->rssi_threshold) + 1) % RSSI_COUNT;
        app->rssi_threshold = rssi_list[idx];
        break;
    }
    }

    view_submenu_reset(app->submenu);
    subghz_scene_settings_on_enter(app);
}

void subghz_scene_settings_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    char buf[40];

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Config", COLOR_BLUE);

    if(app->hopping) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_RIGHT, "Freq  -----",
                              COLOR_DIM, CFG_FREQUENCY, submenu_cb, app);
    } else {
        uint32_t mhz = app->frequency / 1000;
        uint32_t khz = app->frequency % 1000;
        lv_snprintf(buf, sizeof(buf), "Freq  %lu.%03lu",
                    (unsigned long)mhz, (unsigned long)khz);
        view_submenu_add_item(app->submenu, LV_SYMBOL_RIGHT, buf,
                              COLOR_YELLOW, CFG_FREQUENCY, submenu_cb, app);
    }

    lv_snprintf(buf, sizeof(buf), "Hopping  %s", app->hopping ? "ON" : "OFF");
    view_submenu_add_item(app->submenu,
                          app->hopping ? LV_SYMBOL_OK : LV_SYMBOL_RIGHT,
                          buf,
                          app->hopping ? COLOR_GREEN : COLOR_SECONDARY,
                          CFG_HOPPING, submenu_cb, app);

    lv_snprintf(buf, sizeof(buf), "Mod  %s", mod_labels[app->preset]);
    view_submenu_add_item(app->submenu, LV_SYMBOL_RIGHT, buf,
                          COLOR_CYAN, CFG_MODULATION, submenu_cb, app);

    int ri = find_rssi_idx(app->rssi_threshold);
    lv_snprintf(buf, sizeof(buf), "RSSI  %s", rssi_labels[ri]);
    view_submenu_add_item(app->submenu, LV_SYMBOL_RIGHT, buf,
                          COLOR_ORANGE, CFG_RSSI, submenu_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_settings_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_settings_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
}
