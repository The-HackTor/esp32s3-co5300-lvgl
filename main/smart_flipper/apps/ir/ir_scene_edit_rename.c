#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>
#include <string.h>

static void name_accepted(void *ctx, const char *text)
{
    IrApp *app = ctx;
    snprintf(app->name_buffer, sizeof(app->name_buffer), "%s", text);
    scene_manager_handle_custom_event(&app->scene_mgr, IR_EVT_EDIT_NAME_ACCEPTED);
}

void ir_scene_edit_rename_on_enter(void *ctx)
{
    IrApp *app = ctx;

    const char *header = (app->edit_op == IR_EDIT_OP_RENAME_REMOTE)
                         ? "Rename Remote" : "Rename Button";

    view_text_input_reset(app->text_input);
    view_text_input_set_header(app->text_input, header, COLOR_CYAN);
    view_text_input_set_buffer(app->text_input, app->name_buffer,
                               sizeof(app->name_buffer));
    view_text_input_set_callback(app->text_input, name_accepted, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewTextInput,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_edit_rename_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom && event.event == IR_EVT_EDIT_NAME_ACCEPTED) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditRenameDone);
        return true;
    }
    return false;
}

void ir_scene_edit_rename_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_text_input_reset(app->text_input);
}
