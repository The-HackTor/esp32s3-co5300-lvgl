#include "scr_watchface.h"
#include "ui/styles.h"
#include "ui/ui_subjects.h"
#include "ui/arc_menu.h"

#define COLOR_ACCENT  lv_color_hex(0x4FC3F7)
#define COLOR_MUTED   lv_color_hex(0x2A2A2A)

static lv_obj_t *lbl_hour;
static lv_obj_t *lbl_colon;
static lv_obj_t *lbl_min;
static lv_obj_t *lbl_sec;
static lv_obj_t *lbl_date;
static lv_obj_t *lbl_battery;
static lv_obj_t *lbl_steps_val;
static lv_obj_t *lbl_bpm_val;
static lv_obj_t *lbl_temp_val;
static lv_obj_t *ota_overlay;
static lv_obj_t *ota_arc;
static lv_obj_t *ota_lbl;
static bool colon_visible;

static void draw_dot_markers(lv_obj_t *parent)
{
    int cx = DISP_W / 2;
    int cy = DISP_H / 2;

    for(int i = 0; i < 60; i++) {
        int angle_deg = i * 6 - 90;
        int r, size;
        lv_color_t color;

        if(i % 15 == 0) {
            r = 216;
            size = 8;
            color = COLOR_PRIMARY;
        } else if(i % 5 == 0) {
            r = 218;
            size = 5;
            color = COLOR_SECONDARY;
        } else {
            continue;
        }

        int x = cx + (r * lv_trigo_cos(angle_deg)) / 32767;
        int y = cy + (r * lv_trigo_sin(angle_deg)) / 32767;

        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, size, size);
        lv_obj_set_pos(dot, x - size / 2, y - size / 2);
        lv_obj_set_style_bg_color(dot, color, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

static void time_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    (void)subject;
    int h = lv_subject_get_int(&subject_hour);
    int m = lv_subject_get_int(&subject_minute);
    lv_label_set_text_fmt(lbl_hour, "%02d", h);
    lv_label_set_text_fmt(lbl_min, "%02d", m);
}

static void date_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    lv_label_set_text(lbl_date, lv_subject_get_string(subject));
}

static void seconds_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    int s = lv_subject_get_int(subject);
    lv_label_set_text_fmt(lbl_sec, "%02d", s);

    colon_visible = !colon_visible;
    lv_obj_set_style_text_opa(lbl_colon, colon_visible ? LV_OPA_COVER : LV_OPA_30, 0);
}

static void battery_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    int val = lv_subject_get_int(subject);
    lv_label_set_text_fmt(lbl_battery, LV_SYMBOL_BATTERY_FULL "  %d%%", val);
    lv_obj_set_style_text_color(lbl_battery, val <= 20 ? COLOR_RED : COLOR_SECONDARY, 0);
}

static void steps_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    lv_label_set_text_fmt(lbl_steps_val, "%d", lv_subject_get_int(subject));
}

static void temp_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    lv_label_set_text_fmt(lbl_temp_val, "%d\xc2\xb0", lv_subject_get_int(subject));
}

static void ota_observer_cb(lv_observer_t *obs, lv_subject_t *subject)
{
    (void)obs;
    int val = lv_subject_get_int(subject);

    if(val == -1) {
        lv_obj_add_flag(ota_overlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_remove_flag(ota_overlay, LV_OBJ_FLAG_HIDDEN);

    if(val == -2) {
        lv_arc_set_value(ota_arc, 0);
        lv_obj_set_style_arc_color(ota_arc, COLOR_RED, LV_PART_INDICATOR);
        lv_label_set_text(ota_lbl, "Update\nFailed");
        lv_obj_set_style_text_color(ota_lbl, COLOR_RED, 0);
    } else {
        lv_obj_set_style_arc_color(ota_arc, COLOR_ACCENT, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ota_lbl, COLOR_PRIMARY, 0);
        lv_arc_set_value(ota_arc, val);
        lv_label_set_text_fmt(ota_lbl, "Updating\n%d%%", val);
    }
}

static void watchface_clicked(lv_event_t *e)
{
    (void)e;
    arc_menu_show();
}

static void watchface_gesture(lv_event_t *e)
{
    (void)e;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if(dir == LV_DIR_TOP) {
        arc_menu_show();
    }
}

static lv_obj_t *create_complication(lv_obj_t *parent, const char *icon,
                                      const char *label_text, const char *value,
                                      lv_color_t accent, int x_offset)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 110, 80);
    lv_obj_align(cont, LV_ALIGN_CENTER, x_offset, 80);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icn = lv_label_create(cont);
    lv_label_set_text(icn, icon);
    lv_obj_set_style_text_font(icn, FONT_SMALL, 0);
    lv_obj_set_style_text_color(icn, accent, 0);
    lv_obj_align(icn, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *val_lbl = lv_label_create(cont);
    lv_label_set_text(val_lbl, value);
    lv_obj_set_style_text_font(val_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(val_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(val_lbl, LV_ALIGN_CENTER, 0, 2);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label_text);
    lv_obj_set_style_text_font(lbl, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_letter_space(lbl, 1, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);

    return val_lbl;
}

lv_obj_t *scr_watchface_create(void)
{
    static lv_obj_t *s_scr;
    if(s_scr) return s_scr;

    lv_obj_t *scr = lv_obj_create(NULL);
    s_scr = scr;
    lv_obj_add_style(scr, &style_screen, 0);
    lv_obj_set_size(scr, DISP_W, DISP_H);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    draw_dot_markers(scr);

    lbl_battery = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_battery, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_battery, COLOR_SECONDARY, 0);
    lv_label_set_text(lbl_battery, LV_SYMBOL_BATTERY_FULL "  85%");
    lv_obj_align(lbl_battery, LV_ALIGN_TOP_MID, 0, 60);

    lbl_date = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_date, FONT_SMALL, 0);
    lv_obj_set_style_text_color(lbl_date, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_letter_space(lbl_date, 2, 0);
    lv_label_set_text(lbl_date, "");
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, -60);

    lbl_hour = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_hour, FONT_TIME, 0);
    lv_obj_set_style_text_color(lbl_hour, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_letter_space(lbl_hour, 6, 0);
    lv_label_set_text(lbl_hour, "12");
    lv_obj_align(lbl_hour, LV_ALIGN_CENTER, -58, -18);

    lbl_colon = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_colon, FONT_TIME, 0);
    lv_obj_set_style_text_color(lbl_colon, COLOR_ACCENT, 0);
    lv_label_set_text(lbl_colon, ":");
    lv_obj_align(lbl_colon, LV_ALIGN_CENTER, 0, -20);
    colon_visible = true;

    lbl_min = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_min, FONT_TIME, 0);
    lv_obj_set_style_text_color(lbl_min, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_letter_space(lbl_min, 6, 0);
    lv_label_set_text(lbl_min, "00");
    lv_obj_align(lbl_min, LV_ALIGN_CENTER, 58, -18);

    lbl_sec = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_sec, FONT_TITLE, 0);
    lv_obj_set_style_text_color(lbl_sec, COLOR_ACCENT, 0);
    lv_label_set_text(lbl_sec, "00");
    lv_obj_align(lbl_sec, LV_ALIGN_CENTER, 108, -6);

    lv_obj_t *divider = lv_obj_create(scr);
    lv_obj_set_size(divider, 180, 1);
    lv_obj_align(divider, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(divider, COLOR_MUTED, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_remove_flag(divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lbl_steps_val = create_complication(scr, LV_SYMBOL_SHUFFLE, "STEPS",
                                        "0", COLOR_GREEN, -120);
    lbl_bpm_val   = create_complication(scr, LV_SYMBOL_WARNING, "BPM",
                                        "--", COLOR_RED, 0);
    lbl_temp_val  = create_complication(scr, LV_SYMBOL_EYE_OPEN, "TEMP",
                                        "--", COLOR_CYAN, 120);

    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_font(hint, FONT_SMALL, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -48);

    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr, watchface_clicked, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(scr, watchface_gesture, LV_EVENT_GESTURE, NULL);

    lv_subject_add_observer(&subject_time, time_observer_cb, NULL);
    lv_subject_add_observer(&subject_date, date_observer_cb, NULL);
    lv_subject_add_observer(&subject_second, seconds_observer_cb, NULL);
    lv_subject_add_observer(&subject_battery, battery_observer_cb, NULL);
    lv_subject_add_observer(&subject_steps, steps_observer_cb, NULL);
    lv_subject_add_observer(&subject_temperature, temp_observer_cb, NULL);

    ota_overlay = lv_obj_create(scr);
    lv_obj_set_size(ota_overlay, DISP_W, DISP_H);
    lv_obj_center(ota_overlay);
    lv_obj_set_style_bg_color(ota_overlay, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(ota_overlay, LV_OPA_90, 0);
    lv_obj_set_style_border_width(ota_overlay, 0, 0);
    lv_obj_remove_flag(ota_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ota_overlay, LV_OBJ_FLAG_HIDDEN);

    ota_arc = lv_arc_create(ota_overlay);
    lv_obj_set_size(ota_arc, DISP_W - 60, DISP_H - 60);
    lv_obj_center(ota_arc);
    lv_arc_set_range(ota_arc, 0, 100);
    lv_arc_set_value(ota_arc, 0);
    lv_arc_set_bg_angles(ota_arc, 0, 360);
    lv_arc_set_rotation(ota_arc, 270);
    lv_obj_remove_style(ota_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ota_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(ota_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ota_arc, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ota_arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ota_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ota_arc, lv_color_hex(0x1A1A1A), LV_PART_MAIN);

    ota_lbl = lv_label_create(ota_overlay);
    lv_obj_set_style_text_color(ota_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(ota_lbl, FONT_TITLE, 0);
    lv_label_set_text(ota_lbl, "Updating\n0%");
    lv_obj_set_style_text_align(ota_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(ota_lbl);

    lv_subject_add_observer(&subject_ota_progress, ota_observer_cb, NULL);

    return scr;
}
