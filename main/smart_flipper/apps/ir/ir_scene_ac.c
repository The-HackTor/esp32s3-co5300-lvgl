#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "lib/infrared/ac/ac_brand.h"

#include <stdio.h>
#include <stdlib.h>

enum {
    AC_IDX_POWER = 0,
    AC_IDX_MODE,
    AC_IDX_TEMP_DOWN,
    AC_IDX_TEMP_UP,
    AC_IDX_FAN,
    AC_IDX_SWING,
};

static void render(IrApp *app);

static void send_state(IrApp *app)
{
    if(!app->ac_brand || !app->ac_brand->encode) return;
    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    if(app->ac_brand->encode(&app->ac_state, &t, &n, &hz) != ESP_OK) return;
    hw_rgb_set(255, 0, 0);
    hw_ir_send_raw(t, n, hz);
    hw_rgb_off();
    free(t);
}

static void item_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    AcState *s = &app->ac_state;

    switch(index) {
    case AC_IDX_POWER:
        s->power = !s->power;
        break;
    case AC_IDX_MODE:
        s->mode = (AcMode)((s->mode + 1) % AC_MODE_COUNT);
        break;
    case AC_IDX_TEMP_DOWN:
        if(s->temp_c > 16) s->temp_c--;
        break;
    case AC_IDX_TEMP_UP:
        if(s->temp_c < 30) s->temp_c++;
        break;
    case AC_IDX_FAN:
        s->fan = (AcFan)((s->fan + 1) % AC_FAN_COUNT);
        break;
    case AC_IDX_SWING:
        s->swing = !s->swing;
        break;
    default:
        return;
    }
    send_state(app);
    render(app);
}

static void render(IrApp *app)
{
    char power_label[24];
    char mode_label[24];
    char temp_down[24];
    char temp_up[24];
    char fan_label[24];
    char swing_label[24];

    snprintf(power_label, sizeof(power_label), "Power: %s",
             app->ac_state.power ? "On" : "Off");
    snprintf(mode_label, sizeof(mode_label), "Mode: %s",
             ac_mode_label(app->ac_state.mode));
    snprintf(temp_down, sizeof(temp_down), "Temp - (%uC)",
             (unsigned)app->ac_state.temp_c);
    snprintf(temp_up, sizeof(temp_up), "Temp + (%uC)",
             (unsigned)app->ac_state.temp_c);
    snprintf(fan_label, sizeof(fan_label), "Fan: %s",
             ac_fan_label(app->ac_state.fan));
    snprintf(swing_label, sizeof(swing_label), "Swing: %s",
             app->ac_state.swing ? "On" : "Off");

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu,
                            app->ac_brand ? app->ac_brand->name : "AC",
                            COLOR_CYAN);

    view_submenu_add_item(app->submenu, LV_SYMBOL_POWER, power_label,
                          app->ac_state.power ? COLOR_GREEN : COLOR_DIM,
                          AC_IDX_POWER, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, mode_label,
                          COLOR_CYAN, AC_IDX_MODE, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_MINUS, temp_down,
                          COLOR_BLUE, AC_IDX_TEMP_DOWN, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_PLUS, temp_up,
                          COLOR_RED, AC_IDX_TEMP_UP, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_AUDIO, fan_label,
                          COLOR_ORANGE, AC_IDX_FAN, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SHUFFLE, swing_label,
                          app->ac_state.swing ? COLOR_GREEN : COLOR_DIM,
                          AC_IDX_SWING, item_tapped, app);
}

void ir_scene_ac_on_enter(void *ctx)
{
    IrApp *app = ctx;

    if(!app->ac_brand) app->ac_brand = &ac_brand_samsung;
    if(app->ac_state.temp_c < 16 || app->ac_state.temp_c > 30) {
        app->ac_state.temp_c = 24;
        app->ac_state.mode   = AC_MODE_COOL;
        app->ac_state.fan    = AC_FAN_AUTO;
        app->ac_state.swing  = false;
        app->ac_state.power  = false;
    }

    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_ac_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_ac_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
