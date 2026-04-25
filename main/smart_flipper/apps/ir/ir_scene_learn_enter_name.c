#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"

#include <stdio.h>
#include <string.h>

static void name_accepted(void *ctx, const char *text)
{
    IrApp *app = ctx;
    strncpy(app->name_buffer, text, sizeof(app->name_buffer) - 1);
    app->name_buffer[sizeof(app->name_buffer) - 1] = '\0';
    scene_manager_handle_custom_event(&app->scene_mgr, IR_EVT_NAME_ACCEPTED);
}

void ir_scene_learn_enter_name_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_text_input_reset(app->text_input);
    view_text_input_set_header(app->text_input, "Button Name", COLOR_CYAN);

    if(app->name_buffer[0] == '\0') {
        if(app->last_decoded_valid && app->last_decoded.protocol[0] != '\0') {
            snprintf(app->name_buffer, sizeof(app->name_buffer), "%s",
                     app->last_decoded.protocol);
        } else {
            snprintf(app->name_buffer, sizeof(app->name_buffer), "Button%u",
                     (unsigned)(app->current_remote.button_count + 1));
        }
    }
    view_text_input_set_buffer(app->text_input, app->name_buffer,
                               sizeof(app->name_buffer));
    view_text_input_set_callback(app->text_input, name_accepted, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewTextInput);
}

bool ir_scene_learn_enter_name_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom && event.event == IR_EVT_NAME_ACCEPTED) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_LearnDone);
        return true;
    }
    return false;
}

void ir_scene_learn_enter_name_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_text_input_reset(app->text_input);
}
