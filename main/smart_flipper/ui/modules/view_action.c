#include "view_action.h"
#include "ui/styles.h"
#include "ui/widgets/back_button.h"
#include <stdlib.h>

struct ViewAction {
    lv_obj_t *root;
    lv_obj_t *header_bar;
    lv_obj_t *title_lbl;
    lv_obj_t *spinner;
    lv_obj_t *arc;
    lv_obj_t *text_lbl;
    lv_obj_t *detail_lbl;
    lv_obj_t *cancel_btn;
    lv_obj_t *cancel_lbl;
    lv_obj_t *header_detail_lbl;
    lv_anim_t arc_anim;
    bool      arc_anim_active;
    ViewActionCancelCb cancel_cb;
    void              *cancel_ctx;
};

static void arc_anim_cb(void *var, int32_t val)
{
    lv_arc_set_angles((lv_obj_t *)var, (uint32_t)val, (uint32_t)val + 60);
}

static void cancel_clicked(lv_event_t *e)
{
    ViewAction *act = lv_event_get_user_data(e);
    if(act->cancel_cb) act->cancel_cb(act->cancel_ctx);
}

/* --- ViewModule vtable --- */
static lv_obj_t *action_get_view(void *m) { return ((ViewAction *)m)->root; }

static void action_reset(void *m)
{
    ViewAction *a = m;
    lv_label_set_text(a->title_lbl, "");
    lv_label_set_text(a->header_detail_lbl, "");
    lv_label_set_text(a->text_lbl, "");
    lv_label_set_text(a->detail_lbl, "");

    if(a->arc_anim_active) {
        lv_anim_delete(a->arc, arc_anim_cb);
        a->arc_anim_active = false;
    }
    lv_obj_add_flag(a->spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a->arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a->cancel_btn, LV_OBJ_FLAG_HIDDEN);

    a->cancel_cb = NULL;
    a->cancel_ctx = NULL;
}

static void action_destroy(void *m)
{
    ViewAction *a = m;
    if(a->arc_anim_active) lv_anim_delete(a->arc, arc_anim_cb);
    if(a->root) { lv_obj_delete(a->root); a->root = NULL; }
    free(a);
}

static const ViewModuleVtable action_vtable = {
    .get_view = action_get_view,
    .reset    = action_reset,
    .destroy  = action_destroy,
};

ViewAction *view_action_alloc(lv_obj_t *parent)
{
    ViewAction *a = calloc(1, sizeof(ViewAction));

    a->root = lv_obj_create(parent);
    lv_obj_add_flag(a->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(a->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(a->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(a->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(a->root, 0, 0);
    lv_obj_set_style_pad_all(a->root, 0, 0);
    lv_obj_remove_flag(a->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Header bar */
    a->header_bar = lv_obj_create(a->root);
    lv_obj_set_size(a->header_bar, DISP_W, 120);
    lv_obj_align(a->header_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(a->header_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(a->header_bar, 0, 0);
    lv_obj_remove_flag(a->header_bar, LV_OBJ_FLAG_SCROLLABLE);

    back_button_create(a->header_bar);

    a->title_lbl = lv_label_create(a->header_bar);
    lv_label_set_text(a->title_lbl, "");
    lv_obj_set_style_text_font(a->title_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(a->title_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(a->title_lbl, LV_ALIGN_TOP_MID, 0, 20);

    a->header_detail_lbl = lv_label_create(a->header_bar);
    lv_label_set_text(a->header_detail_lbl, "");
    lv_obj_set_style_text_font(a->header_detail_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(a->header_detail_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_align(a->header_detail_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(a->header_detail_lbl, 360);
    lv_obj_align(a->header_detail_lbl, LV_ALIGN_TOP_MID, 0, 54);

    /* Animated arc (rotating indicator) */
    a->arc = lv_arc_create(a->root);
    lv_obj_set_size(a->arc, 150, 150);
    lv_obj_align(a->arc, LV_ALIGN_CENTER, 0, -10);
    lv_arc_set_rotation(a->arc, 0);
    lv_arc_set_bg_angles(a->arc, 0, 360);
    lv_arc_set_angles(a->arc, 0, 60);
    lv_obj_remove_style(a->arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(a->arc, COLOR_RING_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a->arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a->arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(a->arc, true, LV_PART_INDICATOR);
    lv_obj_remove_flag(a->arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(a->arc, LV_OBJ_FLAG_HIDDEN);

    /* Spinner */
    a->spinner = lv_spinner_create(a->root);
    lv_spinner_set_anim_params(a->spinner, 1000, 270);
    lv_obj_set_size(a->spinner, 120, 120);
    lv_obj_align(a->spinner, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_arc_width(a->spinner, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(a->spinner, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a->spinner, COLOR_DIM, LV_PART_MAIN);
    lv_obj_add_flag(a->spinner, LV_OBJ_FLAG_HIDDEN);

    /* Status text -- inside the arc/spinner center */
    a->text_lbl = lv_label_create(a->root);
    lv_label_set_text(a->text_lbl, "");
    lv_obj_set_style_text_font(a->text_lbl, FONT_TIME, 0);
    lv_obj_set_style_text_color(a->text_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_align(a->text_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(a->text_lbl, 120);
    lv_obj_align(a->text_lbl, LV_ALIGN_CENTER, 0, -10);

    /* Detail text -- below the arc with good spacing */
    a->detail_lbl = lv_label_create(a->root);
    lv_label_set_text(a->detail_lbl, "");
    lv_obj_set_style_text_font(a->detail_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(a->detail_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_align(a->detail_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(a->detail_lbl, 380);
    lv_obj_align(a->detail_lbl, LV_ALIGN_CENTER, 0, 90);

    /* Cancel button */
    a->cancel_btn = lv_button_create(a->root);
    lv_obj_set_size(a->cancel_btn, 200, 52);
    lv_obj_set_style_bg_color(a->cancel_btn, COLOR_RED, 0);
    lv_obj_set_style_radius(a->cancel_btn, 26, 0);
    lv_obj_set_style_shadow_width(a->cancel_btn, 0, 0);
    lv_obj_align(a->cancel_btn, LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_add_event_cb(a->cancel_btn, cancel_clicked, LV_EVENT_CLICKED, a);
    lv_obj_add_flag(a->cancel_btn, LV_OBJ_FLAG_HIDDEN);

    a->cancel_lbl = lv_label_create(a->cancel_btn);
    lv_label_set_text(a->cancel_lbl, "Cancel");
    lv_obj_set_style_text_font(a->cancel_lbl, FONT_MENU, 0);
    lv_obj_set_style_text_color(a->cancel_lbl, COLOR_PRIMARY, 0);
    lv_obj_center(a->cancel_lbl);

    return a;
}

void view_action_free(ViewAction *action) { action_destroy(action); }
ViewModule view_action_get_module(ViewAction *action)
{
    return (ViewModule){ .module = action, .vtable = &action_vtable };
}
lv_obj_t *view_action_get_view(ViewAction *action) { return action->root; }
void view_action_reset(ViewAction *action) { action_reset(action); }

void view_action_set_header(ViewAction *a, const char *title, lv_color_t accent)
{
    lv_label_set_text(a->title_lbl, title);
    lv_obj_set_style_text_color(a->title_lbl, accent, 0);
}

void view_action_set_header_detail(ViewAction *a, const char *text)
{
    lv_label_set_text(a->header_detail_lbl, text);
}

void view_action_set_text(ViewAction *a, const char *text)
{
    lv_label_set_text(a->text_lbl, text);
}

void view_action_set_detail(ViewAction *a, const char *text, lv_color_t color)
{
    lv_label_set_text(a->detail_lbl, text);
    lv_obj_set_style_text_color(a->detail_lbl, color, 0);
}

void view_action_show_spinner(ViewAction *a, bool show, lv_color_t color)
{
    if(show) {
        lv_obj_set_style_arc_color(a->spinner, color, LV_PART_INDICATOR);
        lv_obj_remove_flag(a->spinner, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(a->spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_action_show_arc_progress(ViewAction *a, bool show, lv_color_t color)
{
    if(show) {
        lv_obj_set_style_arc_color(a->arc, color, LV_PART_INDICATOR);
        lv_obj_remove_flag(a->arc, LV_OBJ_FLAG_HIDDEN);

        if(!a->arc_anim_active) {
            lv_anim_init(&a->arc_anim);
            lv_anim_set_var(&a->arc_anim, a->arc);
            lv_anim_set_exec_cb(&a->arc_anim, arc_anim_cb);
            lv_anim_set_values(&a->arc_anim, 0, 300);
            lv_anim_set_duration(&a->arc_anim, 2000);
            lv_anim_set_repeat_count(&a->arc_anim, LV_ANIM_REPEAT_INFINITE);
            lv_anim_start(&a->arc_anim);
            a->arc_anim_active = true;
        }
    } else {
        if(a->arc_anim_active) {
            lv_anim_delete(a->arc, arc_anim_cb);
            a->arc_anim_active = false;
        }
        lv_obj_add_flag(a->arc, LV_OBJ_FLAG_HIDDEN);
    }
}

void view_action_set_progress(ViewAction *a, int32_t value)
{
    if(value >= 0 && value <= 360) {
        lv_arc_set_angles(a->arc, 0, (uint32_t)value);
    }
}

void view_action_set_cancel_button(ViewAction *a, const char *text, lv_color_t color,
                                   ViewActionCancelCb cb, void *ctx)
{
    a->cancel_cb = cb;
    a->cancel_ctx = ctx;
    lv_label_set_text(a->cancel_lbl, text);
    lv_obj_set_style_bg_color(a->cancel_btn, color, 0);
    lv_obj_remove_flag(a->cancel_btn, LV_OBJ_FLAG_HIDDEN);
}
