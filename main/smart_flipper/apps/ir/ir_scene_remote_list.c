#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "hw/hw_sleep.h"

#include <stdio.h>
#include <string.h>

#define REMOTE_LIST_MAX 16

static char list_buf[REMOTE_LIST_MAX][IR_REMOTE_NAME_MAX];
static size_t list_count;

static void remote_selected(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= list_count) return;

    char path[IR_REMOTE_PATH_MAX];
    if(ir_store_remote_path(path, sizeof(path), list_buf[index]) != ESP_OK) return;

    ir_remote_free(&app->current_remote);
    if(ir_remote_load(&app->current_remote, path) != ESP_OK) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Load Failed", COLOR_RED);
        view_popup_set_text(app->popup, list_buf[index]);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }
    /* Persist into RTC slow memory so a deep-sleep wake can re-open
     * the same remote and skip the picker. Cleared on remote delete. */
    hw_sleep_set_resume_payload(app->current_remote.path);
    app->is_learning_new_remote = false;
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Remote);
}

static void remote_long_pressed(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= list_count) return;
    snprintf(app->remote_action_target, sizeof(app->remote_action_target),
             "%s", list_buf[index]);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_RemoteActions);
}

void ir_scene_remote_list_on_enter(void *ctx)
{
    IrApp *app = ctx;

    list_count = 0;
    esp_err_t err = ir_store_list_remotes(list_buf, REMOTE_LIST_MAX, &list_count);

    if(err != ESP_OK || list_count == 0) {
        view_popup_reset(app->popup);
        if(err == ESP_ERR_NOT_FOUND) {
            view_popup_set_icon(app->popup, LV_SYMBOL_SD_CARD, COLOR_DIM);
            view_popup_set_header(app->popup, "No SD Card", COLOR_SECONDARY);
            view_popup_set_text(app->popup, "Insert a FAT-formatted card.");
        } else {
            view_popup_set_icon(app->popup, LV_SYMBOL_SD_CARD, COLOR_DIM);
            view_popup_set_header(app->popup, "No Remotes", COLOR_SECONDARY);
            view_popup_set_text(app->popup, "Learn one first.");
        }
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Saved Remotes", COLOR_CYAN);
    for(uint32_t i = 0; i < list_count; i++) {
        char path[IR_REMOTE_PATH_MAX];
        char subtitle[40];
        subtitle[0] = '\0';
        if(ir_store_remote_path(path, sizeof(path), list_buf[i]) == ESP_OK) {
            IrRemoteSummary sum = {0};
            if(ir_remote_summary(path, &sum) == ESP_OK) {
                if(sum.first_protocol[0]) {
                    snprintf(subtitle, sizeof(subtitle), "%u btn  -  %s",
                             (unsigned)sum.button_count, sum.first_protocol);
                } else {
                    snprintf(subtitle, sizeof(subtitle), "%u btn",
                             (unsigned)sum.button_count);
                }
            }
        }
        view_submenu_add_item_ex(app->submenu, LV_SYMBOL_FILE, list_buf[i],
                                 subtitle[0] ? subtitle : NULL,
                                 COLOR_CYAN, i,
                                 remote_selected, remote_long_pressed, app);
    }
    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_RemoteList));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_remote_list_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_remote_list_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
