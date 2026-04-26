#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"

#include <stdio.h>
#include <string.h>

#define LIST_MAX 16

static char  list_buf[LIST_MAX][IR_REMOTE_NAME_MAX];
static size_t list_count;

static void remote_picked(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= list_count) return;
    snprintf(app->pending_step.remote, sizeof(app->pending_step.remote),
             "%s", list_buf[index]);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroPickButton);
}

void ir_scene_macro_pick_remote_on_enter(void *ctx)
{
    IrApp *app = ctx;

    list_count = 0;
    esp_err_t err = ir_store_list_remotes(list_buf, LIST_MAX, &list_count);

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Pick Remote", COLOR_CYAN);

    if(err != ESP_OK || list_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Remotes",
                              COLOR_DIM, 0xFFFFFFFFu, NULL, NULL);
    } else {
        for(uint32_t i = 0; i < list_count; i++) {
            view_submenu_add_item(app->submenu, LV_SYMBOL_FILE, list_buf[i],
                                  COLOR_CYAN, i, remote_picked, app);
        }
    }

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_pick_remote_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_pick_remote_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
