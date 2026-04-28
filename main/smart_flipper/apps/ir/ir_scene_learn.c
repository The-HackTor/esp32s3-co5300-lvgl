#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/widgets/back_button.h"
#include "hw/hw_rgb.h"

#define SCOPE_W 320
#define SCOPE_H 80
#define SCOPE_BAR_CAP 64

static lv_obj_t *s_scope;
static uint32_t  s_last_seen_seq;

static void redraw_scope(IrApp *app)
{
    if(!s_scope) return;
    lv_obj_clean(s_scope);

    if(app->preview_n == 0) return;

    const size_t cap = app->preview_n < SCOPE_BAR_CAP ? app->preview_n : SCOPE_BAR_CAP;
    uint64_t total = 0;
    for(size_t i = 0; i < cap; i++) total += app->preview_timings[i];
    if(total == 0) return;

    const int32_t avail_w = SCOPE_W - 12;
    const int32_t mark_h  = SCOPE_H - 16;
    const int32_t space_h = 4;

    for(size_t i = 0; i < cap; i++) {
        const bool is_mark = (i & 1) == 0;
        int32_t w = (int32_t)(((uint64_t)app->preview_timings[i] * (uint64_t)avail_w) / total);
        if(w < 1) w = 1;

        lv_obj_t *bar = lv_obj_create(s_scope);
        lv_obj_set_size(bar, w, is_mark ? mark_h : space_h);
        lv_obj_set_style_bg_color(bar, is_mark ? COLOR_RED : COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

static void learn_redraw_cb(lv_timer_t *t)
{
    IrApp *app = lv_timer_get_user_data(t);
    if(!app) return;
    if(app->preview_seq == s_last_seen_seq) return;
    s_last_seen_seq = app->preview_seq;
    redraw_scope(app);
}

void ir_scene_learn_on_enter(void *ctx)
{
    IrApp *app = ctx;

    if(app->pending_valid) {
        ir_button_free(&app->pending_button);
        app->pending_valid = false;
    }

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);

    back_button_create(view);

    lv_obj_t *title = lv_label_create(view);
    lv_label_set_text(title, "Listening");
    lv_obj_set_style_text_font(title, FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, COLOR_BLUE, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 64);

    s_scope = lv_obj_create(view);
    lv_obj_set_size(s_scope, SCOPE_W, SCOPE_H);
    lv_obj_align(s_scope, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(s_scope, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(s_scope, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_scope, 8, 0);
    lv_obj_set_style_pad_all(s_scope, 6, 0);
    lv_obj_set_style_border_width(s_scope, 0, 0);
    lv_obj_set_style_pad_column(s_scope, 1, 0);
    lv_obj_set_layout(s_scope, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_scope, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_scope, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(s_scope, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *status = lv_label_create(view);
    lv_label_set_text(status, "Point remote at IR port");
    lv_obj_set_style_text_font(status, FONT_BODY, 0);
    lv_obj_set_style_text_color(status, COLOR_SECONDARY, 0);
    lv_obj_align_to(status, s_scope, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);

    s_last_seen_seq = app->preview_seq;
    redraw_scope(app);

    app->learn_redraw_timer = lv_timer_create(learn_redraw_cb, 100, app);

    /* RX is OWNED by Learn scene -- mirrors Flipper's per-scene worker
     * lifecycle. App boots with RX paused (initial pause in app on_init);
     * Learn enables here and disables on exit so LearnSuccess / EnterName /
     * Done never get spurious RX events that would race with the scene
     * transition or keep mutating app.pending_button while the success
     * view is rendering. */
    ir_app_rx_resume();

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewCustom);
    hw_rgb_set(0, 0, 40);
}

bool ir_scene_learn_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom) {
        if(event.event == IR_EVT_RX_DECODED || event.event == IR_EVT_RX_RAW) {
            scene_manager_next_scene(&app->scene_mgr, ir_SCENE_LearnSuccess);
            return true;
        }
    }
    if(event.type == SceneEventTypeBack && app->is_learning_new_remote &&
       app->current_remote.button_count > 0 && app->current_remote.dirty) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_AskBack);
        return true;
    }
    return false;
}

void ir_scene_learn_on_exit(void *ctx)
{
    IrApp *app = ctx;
    hw_rgb_off();

    /* Stop RX BEFORE tearing down the view: prevents rx_drain_timer_cb from
     * firing IR_EVT_RX_DECODED into the next scene mid-transition and from
     * mutating app.pending_button while LearnSuccess is still rendering. */
    ir_app_rx_pause();

    if(app->learn_redraw_timer) {
        lv_timer_delete(app->learn_redraw_timer);
        app->learn_redraw_timer = NULL;
    }
    s_scope = NULL;
    view_custom_clean(app->custom);
}
