#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdlib.h>

static void dialog_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(result == VIEW_DIALOG_RESULT_LEFT) {
        if(app->pending_valid) {
            ir_button_free(&app->pending_button);
            app->pending_valid = false;
        }
        if(app->pending_raw_timings) {
            free(app->pending_raw_timings);
            app->pending_raw_timings = NULL;
            app->pending_raw_n = 0;
        }
        app->last_decoded_valid = false;

        if(app->is_learning_new_remote) {
            ir_remote_free(&app->current_remote);
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, ir_SCENE_Start);
        } else {
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, ir_SCENE_Remote);
        }
    } else {
        scene_manager_previous_scene(&app->scene_mgr);
    }
}

void ir_scene_ask_back_on_enter(void *ctx)
{
    IrApp *app = ctx;

    const char *header = app->is_learning_new_remote
                             ? "Exit to IR Menu?"
                             : "Exit to Remote Menu?";

    view_dialog_reset(app->dialog);
    view_dialog_set_icon(app->dialog, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_dialog_set_header(app->dialog, header, COLOR_YELLOW);
    view_dialog_set_text(app->dialog, "Unsaved data will be lost.");
    view_dialog_set_left_button(app->dialog, "Exit", COLOR_RED);
    view_dialog_set_right_button(app->dialog, "Stay", COLOR_GREEN);
    view_dialog_set_callback(app->dialog, dialog_cb, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewDialog,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_ask_back_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_ask_back_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_dialog_reset(app->dialog);
}
