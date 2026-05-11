#include "scr_face.h"
#include "ui/styles.h"
#include "ui/assets/img_face.h"
#include "app/app_manager.h"

static void face_clicked(lv_event_t *e)
{
    (void)e;
    app_manager_launch(APP_ID_IR);
}

lv_obj_t *scr_face_create(void)
{
    static lv_obj_t *s_scr;
    if(s_scr) return s_scr;

    lv_obj_t *scr = lv_obj_create(NULL);
    s_scr = scr;

    lv_obj_set_size(scr, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, &img_face);
    lv_obj_center(img);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(scr, face_clicked, LV_EVENT_CLICKED, NULL);

    return scr;
}

void scr_face_show(void)
{
    lv_screen_load(scr_face_create());
}
