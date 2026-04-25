#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "hw/hw_rgb.h"

void ir_scene_learn_on_enter(void *ctx)
{
    IrApp *app = ctx;

    if(app->pending_valid) {
        ir_button_free(&app->pending_button);
        app->pending_valid = false;
    }

    view_action_reset(app->action);
    view_action_set_header(app->action, "Listening", COLOR_BLUE);
    view_action_set_text(app->action, LV_SYMBOL_DOWNLOAD);
    view_action_set_detail(app->action, "Point remote at IR port",
                           COLOR_SECONDARY);
    view_action_show_spinner(app->action, true, COLOR_BLUE);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewAction);
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
    view_action_reset(app->action);
}
