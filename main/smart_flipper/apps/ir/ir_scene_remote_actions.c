#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#define TAG "ir_remote_act"

static char s_dup_name[IR_REMOTE_NAME_MAX];

static void dup_name_accepted(void *ctx, const char *text)
{
    IrApp *app = ctx;
    strncpy(s_dup_name, text, sizeof(s_dup_name) - 1);
    s_dup_name[sizeof(s_dup_name) - 1] = '\0';
    scene_manager_handle_custom_event(&app->scene_mgr,
                                      IR_EVT_DUPLICATE_NAME_ACCEPTED);
}

static void on_open(void *ctx, uint32_t idx)
{
    (void)idx;
    IrApp *app = ctx;
    char path[IR_REMOTE_PATH_MAX];
    if(ir_store_remote_path(path, sizeof(path),
                            app->remote_action_target) != ESP_OK) return;
    ir_remote_free(&app->current_remote);
    if(ir_remote_load(&app->current_remote, path) != ESP_OK) return;
    app->is_learning_new_remote = false;
    /* Replace this scene with Remote so back-gesture goes RemoteList. */
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                      ir_SCENE_RemoteList);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Remote);
}

static void on_rename(void *ctx, uint32_t idx)
{
    (void)idx;
    IrApp *app = ctx;
    char path[IR_REMOTE_PATH_MAX];
    if(ir_store_remote_path(path, sizeof(path),
                            app->remote_action_target) != ESP_OK) return;
    ir_remote_free(&app->current_remote);
    if(ir_remote_load(&app->current_remote, path) != ESP_OK) return;

    app->edit_op = IR_EDIT_OP_RENAME_REMOTE;
    snprintf(app->name_buffer, sizeof(app->name_buffer), "%s",
             app->current_remote.name);
    /* Pop to RemoteList, then push EditRename. */
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                      ir_SCENE_RemoteList);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditRename);
}

static void on_duplicate(void *ctx, uint32_t idx)
{
    (void)idx;
    IrApp *app = ctx;
    snprintf(s_dup_name, sizeof(s_dup_name), "%.20s_Copy",
             app->remote_action_target);

    view_text_input_reset(app->text_input);
    view_text_input_set_header(app->text_input, "New Name", COLOR_CYAN);
    view_text_input_set_buffer(app->text_input, s_dup_name, sizeof(s_dup_name));
    view_text_input_set_callback(app->text_input, dup_name_accepted, app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher,
                                            IrViewTextInput,
                                            (uint32_t)TransitionSlideLeft, 180);
}

static void on_delete(void *ctx, uint32_t idx)
{
    (void)idx;
    IrApp *app = ctx;
    char path[IR_REMOTE_PATH_MAX];
    if(ir_store_remote_path(path, sizeof(path),
                            app->remote_action_target) != ESP_OK) return;

    IrRemote r = {0};
    if(ir_remote_load(&r, path) == ESP_OK) {
        ir_remote_delete_file(&r);
        ir_remote_free(&r);
    }
    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_TRASH, COLOR_RED);
    view_popup_set_header(app->popup, "Deleted", COLOR_RED);
    view_popup_set_text(app->popup, app->remote_action_target);
    view_popup_set_timeout(app->popup, 800, NULL, NULL);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                      ir_SCENE_RemoteList);
}

void ir_scene_remote_actions_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, app->remote_action_target, COLOR_ORANGE);

    view_submenu_add_item(app->submenu, LV_SYMBOL_OK,    "Open",      COLOR_GREEN,  0, on_open,      app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_EDIT,  "Rename",    COLOR_CYAN,   1, on_rename,    app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_COPY,  "Duplicate", COLOR_YELLOW, 2, on_duplicate, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Delete",    COLOR_RED,    3, on_delete,    app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_remote_actions_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom &&
       event.event == IR_EVT_DUPLICATE_NAME_ACCEPTED) {
        char path[IR_REMOTE_PATH_MAX];
        if(ir_store_remote_path(path, sizeof(path),
                                app->remote_action_target) == ESP_OK) {
            IrRemote src = {0};
            if(ir_remote_load(&src, path) == ESP_OK) {
                esp_err_t err = ir_remote_duplicate(&src, s_dup_name);
                ir_remote_free(&src);
                view_popup_reset(app->popup);
                if(err == ESP_OK) {
                    view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
                    view_popup_set_header(app->popup, "Duplicated", COLOR_GREEN);
                    view_popup_set_text(app->popup, s_dup_name);
                } else {
                    view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
                    view_popup_set_header(app->popup, "Failed", COLOR_RED);
                    view_popup_set_text(app->popup, esp_err_to_name(err));
                }
                view_popup_set_timeout(app->popup, 1000, NULL, NULL);
                view_dispatcher_switch_to_view_animated(app->view_dispatcher,
                                                        IrViewPopup,
                                                        (uint32_t)TransitionFadeIn, 120);
            }
        }
        scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                          ir_SCENE_RemoteList);
        return true;
    }
    return false;
}

void ir_scene_remote_actions_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_text_input_reset(app->text_input);
}
