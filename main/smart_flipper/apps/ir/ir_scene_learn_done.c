#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void popup_done(void *ctx)
{
    IrApp *app = ctx;
    if(app->is_learning_new_remote) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Learn);
    } else {
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, ir_SCENE_Remote);
    }
}

void ir_scene_learn_done_on_enter(void *ctx)
{
    IrApp *app = ctx;

    if(app->is_learning_new_remote && app->current_remote.name[0] == '\0') {
        snprintf(app->current_remote.name,
                 sizeof(app->current_remote.name), "%s", app->name_buffer);
        ir_store_remote_path(app->current_remote.path,
                             sizeof(app->current_remote.path),
                             app->current_remote.name);
    }

    if(app->pending_valid) {
        snprintf(app->pending_button.name,
                 sizeof(app->pending_button.name), "%s", app->name_buffer);

        ir_remote_append_button(&app->current_remote, &app->pending_button);
        ir_button_free(&app->pending_button);
        app->pending_valid = false;
    }
    if(app->pending_raw_timings) {
        free(app->pending_raw_timings);
        app->pending_raw_timings = NULL;
        app->pending_raw_n = 0;
    }

    esp_err_t err = ir_remote_save(&app->current_remote);

    view_popup_reset(app->popup);
    if(err == ESP_OK) {
        app->current_remote.dirty = false;
        view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
        view_popup_set_header(app->popup, "Saved", COLOR_GREEN);
        view_popup_set_text(app->popup, app->current_remote.name);
    } else {
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Save Failed", COLOR_RED);
        view_popup_set_text(app->popup, "Check SD card.");
    }
    view_popup_set_timeout(app->popup, 1500, popup_done, app);

    memset(app->name_buffer, 0, sizeof(app->name_buffer));

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
}

bool ir_scene_learn_done_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_learn_done_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_popup_reset(app->popup);
}
