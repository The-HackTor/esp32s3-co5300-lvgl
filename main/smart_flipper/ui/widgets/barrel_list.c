#include "barrel_list.h"
#include "ui/styles.h"
#include "ui/circular_layout.h"

static void apply_barrel(lv_obj_t *list)
{
    int32_t center_y = DISP_H / 2;
    uint32_t count = lv_obj_get_child_count(list);

    for(uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(list, i);
        lv_area_t a;
        lv_obj_get_coords(child, &a);

        int32_t dy = (a.y1 + a.y2) / 2 - center_y;
        int32_t curve = circular_curve_factor(dy);

        int32_t w = (DISP_W - 40) * curve / 255;
        if(w < 100) w = 100;
        lv_obj_set_width(child, w);

        int32_t opa = curve;
        if(opa < 60) opa = 60;
        lv_obj_set_style_opa(child, opa, 0);
    }
}

static void scroll_cb(lv_event_t *e)
{
    apply_barrel(lv_event_get_target(e));
}

lv_obj_t *barrel_list_create(lv_obj_t *parent)
{
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, DISP_W, DISP_H);
    lv_obj_center(list);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_style_pad_top(list, 50, 0);
    lv_obj_set_style_pad_bottom(list, 80, 0);
    lv_obj_set_style_pad_left(list, 20, 0);
    lv_obj_set_style_pad_right(list, 20, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_event_cb(list, scroll_cb, LV_EVENT_SCROLL, NULL);
    return list;
}

void barrel_list_refresh(lv_obj_t *list)
{
    lv_obj_update_layout(list);
    apply_barrel(list);
}
