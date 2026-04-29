#include "status_bar.h"
#include "back_button.h"
#include "ui/styles.h"
#include "hw/hw_bat.h"

#include <stdio.h>

static void bat_label_deleted(lv_event_t *e)
{
    lv_timer_t *t = lv_event_get_user_data(e);
    if(t) lv_timer_delete(t);
}

static void bat_refresh_cb(lv_timer_t *t)
{
    lv_obj_t *lbl = lv_timer_get_user_data(t);
    if(!lbl) return;

    int pct = hw_bat_read_soc_pct();
    int mv  = hw_bat_read_mv();
    bool charging = hw_bat_is_charging();

    /* Pick a glyph by SoC bin so the user reads battery state at a
     * glance even before parsing the percentage digits. */
    const char *glyph;
    if(charging)         glyph = LV_SYMBOL_CHARGE;
    else if(pct >= 80)   glyph = LV_SYMBOL_BATTERY_FULL;
    else if(pct >= 60)   glyph = LV_SYMBOL_BATTERY_3;
    else if(pct >= 40)   glyph = LV_SYMBOL_BATTERY_2;
    else if(pct >= 20)   glyph = LV_SYMBOL_BATTERY_1;
    else                 glyph = LV_SYMBOL_BATTERY_EMPTY;

    lv_color_t color;
    if(charging)         color = COLOR_GREEN;
    else if(pct <= 15)   color = COLOR_RED;
    else if(pct <= 30)   color = COLOR_YELLOW;
    else                 color = COLOR_PRIMARY;
    lv_obj_set_style_text_color(lbl, color, 0);

    if(mv > 0) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%s %d%%", glyph, pct);
        lv_label_set_text(lbl, buf);
    } else {
        lv_label_set_text(lbl, glyph);
    }
}

lv_obj_t *status_bar_create(lv_obj_t *parent, const char *title, lv_color_t accent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, DISP_W, 70);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    back_button_create(bar);

    lv_obj_t *lbl = lv_label_create(bar);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(lbl, accent, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 24);

    /* Battery glyph + percentage on the right. The refresh timer is
     * scoped to the label widget's lifetime via LV_EVENT_DELETE so a
     * scene tear-down kills the timer with the bar. */
    lv_obj_t *bat = lv_label_create(bar);
    lv_obj_set_style_text_font(bat, FONT_BODY, 0);
    lv_obj_set_style_text_color(bat, COLOR_PRIMARY, 0);
    lv_obj_align(bat, LV_ALIGN_TOP_RIGHT, -16, 28);
    lv_label_set_text(bat, "");

    lv_timer_t *t = lv_timer_create(bat_refresh_cb, 1000, bat);
    lv_timer_ready(t);
    lv_obj_add_event_cb(bat, bat_label_deleted, LV_EVENT_DELETE, t);

    return bar;
}
