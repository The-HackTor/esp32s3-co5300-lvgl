#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>
#include <string.h>

static void popup_done(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_search_and_switch_to_previous_scene(
        &app->scene_mgr, ir_SCENE_Remote);
}

void ir_scene_edit_rename_done_on_enter(void *ctx)
{
    IrApp *app = ctx;

    esp_err_t err = ESP_FAIL;
    const char *new_name = app->name_buffer;

    if(app->edit_op == IR_EDIT_OP_RENAME_REMOTE) {
        if(new_name[0] != '\0') {
            err = ir_remote_rename(&app->current_remote, new_name);
        }
    } else if(app->edit_op == IR_EDIT_OP_RENAME_BUTTON) {
        if(app->selected_button_idx >= 0 && new_name[0] != '\0') {
            err = ir_remote_rename_button(&app->current_remote,
                                          (size_t)app->selected_button_idx,
                                          new_name);
            if(err == ESP_OK) err = ir_remote_save(&app->current_remote);
        }
    }

    view_popup_reset(app->popup);
    if(err == ESP_OK) {
        app->current_remote.dirty = false;
        view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
        view_popup_set_header(app->popup, "Renamed", COLOR_GREEN);
        view_popup_set_text(app->popup, new_name);
    } else if(err == ESP_ERR_INVALID_STATE) {
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
        view_popup_set_header(app->popup, "Name in Use", COLOR_YELLOW);
        view_popup_set_text(app->popup,
            "A remote with that name already exists.");
    } else {
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Rename Failed", COLOR_RED);
        view_popup_set_text(app->popup, "Check SD card.");
    }
    view_popup_set_timeout(app->popup, 1500, popup_done, app);

    app->edit_op = IR_EDIT_OP_NONE;
    app->selected_button_idx = -1;
    memset(app->name_buffer, 0, sizeof(app->name_buffer));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
}

bool ir_scene_edit_rename_done_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_edit_rename_done_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_popup_reset(app->popup);
}
