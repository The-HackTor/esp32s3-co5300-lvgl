#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <string.h>

static IrEditOp s_pending_op;

static void popup_done(void *ctx)
{
    IrApp *app = ctx;
    if(s_pending_op == IR_EDIT_OP_DELETE_REMOTE) {
        ir_remote_free(&app->current_remote);
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, ir_SCENE_Start);
    } else {
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, ir_SCENE_Remote);
    }
    s_pending_op = IR_EDIT_OP_NONE;
}

void ir_scene_edit_delete_done_on_enter(void *ctx)
{
    IrApp *app = ctx;

    s_pending_op = (IrEditOp)app->edit_op;
    esp_err_t err = ESP_FAIL;

    if(app->edit_op == IR_EDIT_OP_DELETE_REMOTE) {
        err = ir_remote_delete_file(&app->current_remote);
    } else if(app->edit_op == IR_EDIT_OP_DELETE_BUTTON
              && app->selected_button_idx >= 0) {
        err = ir_remote_delete_button(&app->current_remote,
                                      (size_t)app->selected_button_idx);
        if(err == ESP_OK) err = ir_remote_save(&app->current_remote);
    }

    view_popup_reset(app->popup);
    if(err == ESP_OK) {
        view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
        view_popup_set_header(app->popup, "Deleted", COLOR_GREEN);
        view_popup_set_text(app->popup,
            (s_pending_op == IR_EDIT_OP_DELETE_REMOTE) ? "Remote removed."
                                                       : "Button removed.");
    } else {
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Delete Failed", COLOR_RED);
        view_popup_set_text(app->popup, "Check SD card.");
    }
    view_popup_set_timeout(app->popup, 1500, popup_done, app);

    app->edit_op = IR_EDIT_OP_NONE;
    app->selected_button_idx = -1;

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
}

bool ir_scene_edit_delete_done_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_edit_delete_done_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_popup_reset(app->popup);
}
