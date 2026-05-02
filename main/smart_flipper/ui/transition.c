#include "transition.h"
#include "styles.h"

static void anim_opa(void *obj, int32_t val)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)val, 0);
}

static void anim_hide_on_done(lv_anim_t *a)
{
    lv_obj_t *obj = lv_anim_get_user_data(a);
    if(obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void transition_apply(lv_obj_t *outgoing, lv_obj_t *incoming,
                      TransitionType type, uint32_t duration_ms)
{
    if(type == TransitionNone) return;

    if(type == TransitionFadeIn) {
        if(incoming) {
            lv_obj_set_style_opa(incoming, LV_OPA_TRANSP, 0);
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, incoming);
            lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
            lv_anim_set_duration(&a, duration_ms);
            lv_anim_set_exec_cb(&a, anim_opa);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_start(&a);
        }
        if(outgoing) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, outgoing);
            lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
            lv_anim_set_duration(&a, duration_ms);
            lv_anim_set_exec_cb(&a, anim_opa);
            lv_anim_set_user_data(&a, outgoing);
            lv_anim_set_completed_cb(&a, anim_hide_on_done);
            lv_anim_start(&a);
        }
        return;
    }

    int32_t offset = (type == TransitionSlideLeft) ? DISP_W : -DISP_W;

    if(incoming) {
        lv_obj_set_x(incoming, offset);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, incoming);
        lv_anim_set_values(&a, offset, 0);
        lv_anim_set_duration(&a, duration_ms);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }

    if(outgoing) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, outgoing);
        lv_anim_set_values(&a, 0, -offset);
        lv_anim_set_duration(&a, duration_ms);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_user_data(&a, outgoing);
        lv_anim_set_completed_cb(&a, anim_hide_on_done);
        lv_anim_start(&a);
    }
}
