#include "scr_settings.h"
#include "ui/styles.h"
#include "ui/ui_subjects.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/widgets/back_button.h"
#include "bridge/bridge.h"
#include "smart_flipper/smart_flipper.h"
#include <app/bridge_protocol.h>

static lv_obj_t *screen;
static lv_obj_t *lbl_bright_val;
static lv_obj_t *lbl_batt;
static lv_obj_t *lbl_ble;
static lv_obj_t *ble_dot;

static void brightness_changed(lv_event_t *e)
{
    lv_obj_t *arc = lv_event_get_target(e);
    int val = lv_arc_get_value(arc);
    lv_label_set_text_fmt(lbl_bright_val, "%d%%", (val * 100) / 255);
    uint8_t level = (uint8_t)val;
    lv_subject_set_int(&subject_brightness, level);
    app_brightness_set(level);
    bridge_send(BRIDGE_MSG_BRIGHTNESS, &level, 1);
}

static void find_phone_cb(lv_event_t *e)
{
    (void)e;
    bridge_send(BRIDGE_MSG_FIND_PHONE, NULL, 0);
}

static void batt_observer_cb(lv_observer_t *o, lv_subject_t *s)
{
    (void)o;
    int val = lv_subject_get_int(s);
    lv_label_set_text_fmt(lbl_batt, "%d%%", val);
    lv_obj_set_style_text_color(lbl_batt, val > 20 ? COLOR_GREEN : COLOR_RED, 0);
}

static void ble_observer_cb(lv_observer_t *o, lv_subject_t *s)
{
    (void)o;
    int state = lv_subject_get_int(s);
    const char *str = state == 2 ? "Connected" : state == 1 ? "On" : "Off";
    lv_color_t color = state == 2 ? COLOR_BLUE : state == 1 ? COLOR_CYAN : COLOR_DIM;
    lv_label_set_text(lbl_ble, str);
    lv_obj_set_style_text_color(lbl_ble, color, 0);
    lv_obj_set_style_bg_color(ble_dot, color, 0);
}

static lv_obj_t *create_row(lv_obj_t *parent, const char *icon, const char *label,
                             lv_color_t icon_color, int y)
{
    lv_obj_t *icn = lv_label_create(parent);
    lv_label_set_text(icn, icon);
    lv_obj_set_style_text_font(icn, FONT_TITLE, 0);
    lv_obj_set_style_text_color(icn, icon_color, 0);
    lv_obj_set_pos(icn, 80, y);

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_set_pos(lbl, 120, y + 2);

    return lbl;
}

static void create_sep(lv_obj_t *parent, int y)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, 280, 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

static void on_init(void)
{
    if(screen) return;
    screen = lv_obj_create(NULL);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, DISP_W, DISP_H);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    nav_install_gesture(screen);

    back_button_create(screen);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, COLOR_SECONDARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *arc = lv_arc_create(screen);
    lv_obj_set_size(arc, 130, 130);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, -60);
    lv_arc_set_range(arc, 10, 255);
    lv_arc_set_value(arc, lv_subject_get_int(&subject_brightness));
    lv_arc_set_bg_angles(arc, 135, 45);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COLOR_RING_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, COLOR_YELLOW, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, COLOR_YELLOW, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 3, LV_PART_KNOB);
    lv_obj_add_event_cb(arc, brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);

    int bright_pct = (lv_subject_get_int(&subject_brightness) * 100) / 255;
    lbl_bright_val = lv_label_create(screen);
    lv_label_set_text_fmt(lbl_bright_val, "%d%%", bright_pct);
    lv_obj_set_style_text_font(lbl_bright_val, FONT_TITLE, 0);
    lv_obj_set_style_text_color(lbl_bright_val, COLOR_PRIMARY, 0);
    lv_obj_align(lbl_bright_val, LV_ALIGN_CENTER, 0, -60);

    lv_obj_t *bright_lbl = lv_label_create(screen);
    lv_label_set_text(bright_lbl, "Brightness");
    lv_obj_set_style_text_font(bright_lbl, FONT_TINY, 0);
    lv_obj_set_style_text_color(bright_lbl, COLOR_YELLOW, 0);
    lv_obj_align(bright_lbl, LV_ALIGN_CENTER, 0, -30);

    int row_y = 252;
    int row_h = 36;

    create_row(screen, LV_SYMBOL_BATTERY_FULL, "Battery", COLOR_GREEN, row_y);
    lbl_batt = lv_label_create(screen);
    lv_label_set_text(lbl_batt, "0%");
    lv_obj_set_style_text_font(lbl_batt, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl_batt, COLOR_GREEN, 0);
    lv_obj_set_pos(lbl_batt, 340, row_y + 2);

    create_sep(screen, row_y + row_h);
    row_y += row_h + 6;

    create_row(screen, LV_SYMBOL_BLUETOOTH, "Bluetooth", COLOR_BLUE, row_y);
    ble_dot = lv_obj_create(screen);
    lv_obj_set_size(ble_dot, 8, 8);
    lv_obj_set_style_radius(ble_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ble_dot, COLOR_DIM, 0);
    lv_obj_set_style_bg_opa(ble_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ble_dot, 0, 0);
    lv_obj_set_pos(ble_dot, 340, row_y + 8);
    lv_obj_remove_flag(ble_dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lbl_ble = lv_label_create(screen);
    lv_label_set_text(lbl_ble, "Off");
    lv_obj_set_style_text_font(lbl_ble, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl_ble, COLOR_DIM, 0);
    lv_obj_set_pos(lbl_ble, 354, row_y + 2);

    create_sep(screen, row_y + row_h);
    row_y += row_h + 6;

    create_row(screen, LV_SYMBOL_SETTINGS, "CO5300 v1.0", COLOR_SECONDARY, row_y);

    create_sep(screen, row_y + row_h);
    row_y += row_h + 6;

    lv_obj_t *find_btn = lv_obj_create(screen);
    lv_obj_set_size(find_btn, 200, 44);
    lv_obj_align(find_btn, LV_ALIGN_BOTTOM_MID, 0, -38);
    lv_obj_set_style_bg_color(find_btn, COLOR_CYAN, 0);
    lv_obj_set_style_bg_opa(find_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(find_btn, 22, 0);
    lv_obj_set_style_border_width(find_btn, 0, 0);
    lv_obj_add_flag(find_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(find_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(find_btn, find_phone_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *find_lbl = lv_label_create(find_btn);
    lv_label_set_text(find_lbl, LV_SYMBOL_GPS "  Find Phone");
    lv_obj_set_style_text_font(find_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(find_lbl, COLOR_BG, 0);
    lv_obj_center(find_lbl);

    lv_subject_add_observer(&subject_battery, batt_observer_cb, NULL);
    lv_subject_add_observer(&subject_bluetooth, ble_observer_cb, NULL);
}

static void on_enter(void) { lv_screen_load(screen); }
static void on_leave(void) {}
static lv_obj_t *get_screen(void) { return screen; }

void scr_settings_register(void)
{
    static const AppDescriptor desc = {
        .id = APP_ID_SETTINGS,
        .name = "Settings",
        .icon = LV_SYMBOL_SETTINGS,
        .color = {.red = 0x88, .green = 0x88, .blue = 0x88},
        .on_init = on_init,
        .on_enter = on_enter,
        .on_leave = on_leave,
        .get_screen = get_screen,
        .get_scene_manager = NULL,
    };
    app_manager_register(&desc);
}
