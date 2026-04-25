#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"

static void dialog_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(result == VIEW_DIALOG_RESULT_LEFT) {
        ir_remote_free(&app->current_remote);
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, ir_SCENE_Start);
    } else {
        scene_manager_previous_scene(&app->scene_mgr);
    }
}

void ir_scene_ask_back_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_dialog_reset(app->dialog);
    view_dialog_set_icon(app->dialog, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_dialog_set_header(app->dialog, "Discard Remote?", COLOR_YELLOW);
    view_dialog_set_text(app->dialog, "Unsaved buttons will be lost.");
    view_dialog_set_left_button(app->dialog, "Discard", COLOR_RED);
    view_dialog_set_right_button(app->dialog, "Keep", COLOR_GREEN);
    view_dialog_set_callback(app->dialog, dialog_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewDialog);
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
