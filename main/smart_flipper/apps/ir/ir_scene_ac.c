#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "ui/widgets/back_button.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "store/ir_store.h"
#include "lib/infrared/ac/ac_brand.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AC_SEND_DEBOUNCE_MS 150

static lv_obj_t *s_temp_lbl;
static lv_obj_t *s_mode_lbl;
static lv_obj_t *s_fan_lbl;
static lv_obj_t *s_swing_lbl;
static lv_obj_t *s_power_lbl;
static lv_obj_t *s_brand_lbl;

static const char *mode_icon(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return LV_SYMBOL_REFRESH;
    case AC_MODE_COOL: return LV_SYMBOL_DOWNLOAD;
    case AC_MODE_DRY:  return LV_SYMBOL_TINT;
    case AC_MODE_FAN:  return LV_SYMBOL_LOOP;
    case AC_MODE_HEAT: return LV_SYMBOL_UPLOAD;
    default:           return LV_SYMBOL_REFRESH;
    }
}

static const char *fan_bars(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return "Auto";
    case AC_FAN_LOW:  return "▁";
    case AC_FAN_MED:  return "▁▂";
    case AC_FAN_HIGH: return "▁▂▃";
    default:          return "?";
    }
}

static void update_labels(IrApp *app)
{
    if(!s_temp_lbl) return;
    char buf[24];
    snprintf(buf, sizeof(buf), "%u°C", (unsigned)app->ac_state.temp_c);
    lv_label_set_text(s_temp_lbl, buf);

    snprintf(buf, sizeof(buf), "%s %s",
             mode_icon(app->ac_state.mode), ac_mode_label(app->ac_state.mode));
    lv_label_set_text(s_mode_lbl, buf);

    snprintf(buf, sizeof(buf), "Fan: %s", fan_bars(app->ac_state.fan));
    lv_label_set_text(s_fan_lbl, buf);

    snprintf(buf, sizeof(buf), "Swing: %s",
             app->ac_state.swing ? "On" : "Off");
    lv_label_set_text(s_swing_lbl, buf);

    snprintf(buf, sizeof(buf), "Power: %s",
             app->ac_state.power ? "ON" : "Off");
    lv_label_set_text(s_power_lbl, buf);
    lv_obj_set_style_text_color(s_power_lbl,
                                app->ac_state.power ? COLOR_GREEN : COLOR_DIM, 0);

    if(s_brand_lbl) {
        lv_label_set_text(s_brand_lbl,
                          app->ac_brand ? app->ac_brand->name : "AC");
    }
}

static void send_debounced(IrApp *app);

static void send_now(lv_timer_t *t)
{
    IrApp *app = lv_timer_get_user_data(t);
    lv_timer_delete(t);
    app->ac_send_timer = NULL;

    if(!app->ac_brand || !app->ac_brand->encode) return;

    uint16_t *buf = NULL;
    size_t    n   = 0;
    uint32_t  hz  = 38000;
    if(app->ac_brand->encode(&app->ac_state, &buf, &n, &hz) != ESP_OK) return;

    hw_rgb_set(255, 0, 0);
    hw_ir_send_raw(buf, n, hz);
    hw_rgb_off();
    free(buf);

    if(app->ac_brand && app->ac_brand->name) {
        ir_ac_state_save(app->ac_brand->name,
                         app->ac_state.power, (uint8_t)app->ac_state.mode,
                         app->ac_state.temp_c, (uint8_t)app->ac_state.fan,
                         app->ac_state.swing);
    }
    app->ac_dirty = false;
}

static void send_debounced(IrApp *app)
{
    app->ac_dirty = true;
    if(app->ac_send_timer) {
        lv_timer_reset(app->ac_send_timer);
    } else {
        app->ac_send_timer = lv_timer_create(send_now, AC_SEND_DEBOUNCE_MS, app);
        lv_timer_set_repeat_count(app->ac_send_timer, 1);
    }
}

static void on_power(lv_event_t *e)
{
    IrApp *app = lv_event_get_user_data(e);
    app->ac_state.power = !app->ac_state.power;
    update_labels(app);
    send_debounced(app);
}

static void on_temp(lv_event_t *e)
{
    IrApp *app = lv_event_get_user_data(e);
    int delta = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target_obj(e));
    int t = (int)app->ac_state.temp_c + delta;
    if(t < 16) t = 16;
    if(t > 30) t = 30;
    app->ac_state.temp_c = (uint8_t)t;
    update_labels(app);
    send_debounced(app);
}

static void on_mode(lv_event_t *e)
{
    IrApp *app = lv_event_get_user_data(e);
    app->ac_state.mode = (AcMode)((app->ac_state.mode + 1) % AC_MODE_COUNT);
    update_labels(app);
    send_debounced(app);
}

static void on_fan(lv_event_t *e)
{
    IrApp *app = lv_event_get_user_data(e);
    app->ac_state.fan = (AcFan)((app->ac_state.fan + 1) % AC_FAN_COUNT);
    update_labels(app);
    send_debounced(app);
}

static void on_swing(lv_event_t *e)
{
    IrApp *app = lv_event_get_user_data(e);
    app->ac_state.swing = !app->ac_state.swing;
    update_labels(app);
    send_debounced(app);
}

static lv_obj_t *make_pill(lv_obj_t *parent, const char *text,
                           lv_color_t bg, int32_t w, int32_t h,
                           lv_event_cb_t cb, void *ctx, intptr_t udata)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, h / 2, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_user_data(btn, (void *)udata);
    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_center(lbl);
    return btn;
}

void ir_scene_ac_on_enter(void *ctx)
{
    IrApp *app = ctx;
    if(!app->ac_brand) app->ac_brand = &ac_brand_samsung;

    bool    ld_power, ld_swing;
    uint8_t ld_mode, ld_temp, ld_fan;
    if(app->ac_brand && app->ac_brand->name &&
       ir_ac_state_load(app->ac_brand->name, &ld_power, &ld_mode, &ld_temp,
                        &ld_fan, &ld_swing) == ESP_OK) {
        app->ac_state.power  = ld_power;
        app->ac_state.mode   = (AcMode)ld_mode;
        app->ac_state.temp_c = ld_temp;
        app->ac_state.fan    = (AcFan)ld_fan;
        app->ac_state.swing  = ld_swing;
    } else if(app->ac_state.temp_c < 16 || app->ac_state.temp_c > 30) {
        app->ac_state.power  = false;
        app->ac_state.mode   = AC_MODE_COOL;
        app->ac_state.temp_c = 24;
        app->ac_state.fan    = AC_FAN_AUTO;
        app->ac_state.swing  = false;
    }

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);
    back_button_create(view);

    /* Brand header */
    s_brand_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_brand_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(s_brand_lbl, COLOR_CYAN, 0);
    lv_obj_align(s_brand_lbl, LV_ALIGN_TOP_MID, 0, 36);

    /* Big temp number */
    s_temp_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_temp_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_temp_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(s_temp_lbl, LV_ALIGN_TOP_MID, 0, 100);

    /* Temp -/+ buttons flanking */
    make_pill(view, LV_SYMBOL_MINUS, COLOR_BLUE,  64, 64, on_temp, app, -1);
    {
        lv_obj_t *minus_btn = lv_obj_get_child(view, lv_obj_get_child_count(view) - 1);
        lv_obj_align(minus_btn, LV_ALIGN_TOP_MID, -120, 88);
    }
    make_pill(view, LV_SYMBOL_PLUS, COLOR_RED,    64, 64, on_temp, app,  1);
    {
        lv_obj_t *plus_btn = lv_obj_get_child(view, lv_obj_get_child_count(view) - 1);
        lv_obj_align(plus_btn, LV_ALIGN_TOP_MID, 120, 88);
    }

    /* Mode pill */
    s_mode_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_mode_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(s_mode_lbl, COLOR_CYAN, 0);
    lv_obj_align(s_mode_lbl, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_t *mode_btn = lv_obj_create(view);
    lv_obj_set_size(mode_btn, 200, 38);
    lv_obj_set_style_bg_opa(mode_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_btn, 0, 0);
    lv_obj_align(mode_btn, LV_ALIGN_TOP_MID, 0, 175);
    lv_obj_remove_flag(mode_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(mode_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(mode_btn, on_mode, LV_EVENT_CLICKED, app);

    /* Fan pill */
    s_fan_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_fan_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(s_fan_lbl, COLOR_ORANGE, 0);
    lv_obj_align(s_fan_lbl, LV_ALIGN_TOP_MID, 0, 220);
    lv_obj_t *fan_btn = lv_obj_create(view);
    lv_obj_set_size(fan_btn, 200, 38);
    lv_obj_set_style_bg_opa(fan_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fan_btn, 0, 0);
    lv_obj_align(fan_btn, LV_ALIGN_TOP_MID, 0, 215);
    lv_obj_remove_flag(fan_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(fan_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(fan_btn, on_fan, LV_EVENT_CLICKED, app);

    /* Swing pill */
    s_swing_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_swing_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(s_swing_lbl, COLOR_SECONDARY, 0);
    lv_obj_align(s_swing_lbl, LV_ALIGN_TOP_MID, 0, 260);
    lv_obj_t *swing_btn = lv_obj_create(view);
    lv_obj_set_size(swing_btn, 200, 38);
    lv_obj_set_style_bg_opa(swing_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(swing_btn, 0, 0);
    lv_obj_align(swing_btn, LV_ALIGN_TOP_MID, 0, 255);
    lv_obj_remove_flag(swing_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(swing_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(swing_btn, on_swing, LV_EVENT_CLICKED, app);

    /* Power button at bottom */
    s_power_lbl = lv_label_create(view);
    lv_obj_set_style_text_font(s_power_lbl, FONT_TITLE, 0);
    lv_obj_align(s_power_lbl, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_t *power_btn = lv_obj_create(view);
    lv_obj_set_size(power_btn, 220, 60);
    lv_obj_set_style_bg_opa(power_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(power_btn, 0, 0);
    lv_obj_align(power_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_remove_flag(power_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(power_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(power_btn, on_power, LV_EVENT_CLICKED, app);

    update_labels(app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewCustom,
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
    if(app->ac_send_timer) {
        lv_timer_delete(app->ac_send_timer);
        app->ac_send_timer = NULL;
    }
    if(app->ac_dirty && app->ac_brand && app->ac_brand->name) {
        ir_ac_state_save(app->ac_brand->name,
                         app->ac_state.power, (uint8_t)app->ac_state.mode,
                         app->ac_state.temp_c, (uint8_t)app->ac_state.fan,
                         app->ac_state.swing);
        app->ac_dirty = false;
    }
    s_temp_lbl = s_mode_lbl = s_fan_lbl = s_swing_lbl = s_power_lbl = s_brand_lbl = NULL;
    view_custom_clean(app->custom);
}
