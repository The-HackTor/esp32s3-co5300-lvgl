#include "status_bar.h"
#include "back_button.h"
#include "ui/styles.h"

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

    return bar;
}
