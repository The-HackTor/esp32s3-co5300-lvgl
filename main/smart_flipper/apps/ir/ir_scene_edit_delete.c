#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>

static void dialog_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(result == VIEW_DIALOG_RESULT_LEFT) {
        scene_manager_handle_custom_event(&app->scene_mgr, IR_EVT_EDIT_DELETE_CONFIRMED);
    } else {
        scene_manager_previous_scene(&app->scene_mgr);
    }
}

void ir_scene_edit_delete_on_enter(void *ctx)
{
    IrApp *app = ctx;
    char text[96];

    view_dialog_reset(app->dialog);
    view_dialog_set_icon(app->dialog, LV_SYMBOL_WARNING, COLOR_RED);

    if(app->edit_op == IR_EDIT_OP_DELETE_REMOTE) {
        view_dialog_set_header(app->dialog, "Delete Remote?", COLOR_RED);
        snprintf(text, sizeof(text), "%s and all buttons.", app->current_remote.name);
        view_dialog_set_text(app->dialog, text);
    } else if(app->edit_op == IR_EDIT_OP_DELETE_BUTTON
              && app->selected_button_idx >= 0
              && (size_t)app->selected_button_idx < app->current_remote.button_count) {
        view_dialog_set_header(app->dialog, "Delete Button?", COLOR_RED);
        snprintf(text, sizeof(text), "%s",
                 app->current_remote.buttons[app->selected_button_idx].name);
        view_dialog_set_text(app->dialog, text);
    } else {
        view_dialog_set_header(app->dialog, "Delete?", COLOR_RED);
        view_dialog_set_text(app->dialog, "Confirm.");
    }

    view_dialog_set_left_button(app->dialog, "Delete", COLOR_RED);
    view_dialog_set_right_button(app->dialog, "Keep", COLOR_GREEN);
    view_dialog_set_callback(app->dialog, dialog_cb, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewDialog,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_edit_delete_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom && event.event == IR_EVT_EDIT_DELETE_CONFIRMED) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditDeleteDone);
        return true;
    }
    return false;
}

void ir_scene_edit_delete_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_dialog_reset(app->dialog);
}
