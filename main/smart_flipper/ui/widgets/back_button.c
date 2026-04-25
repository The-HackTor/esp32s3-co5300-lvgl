#include "back_button.h"
#include "ui/styles.h"

static void back_btn_clicked(lv_event_t *e)
{
    (void)e;
    lv_obj_t *scr = lv_screen_active();
    if(scr) lv_obj_send_event(scr, LV_EVENT_GESTURE, NULL);
}

lv_obj_t *back_button_create(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 44, 44);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, COLOR_RING_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 30, 20);
    lv_obj_add_event_cb(btn, back_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_center(lbl);

    return btn;
}
