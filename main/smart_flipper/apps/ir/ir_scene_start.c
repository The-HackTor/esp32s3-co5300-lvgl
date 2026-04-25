#include "ir_app.h"
#include "ir_scenes.h"
#include "app/app_manager.h"
#include "ui/styles.h"

enum { IDX_UNIVERSAL, IDX_LEARN, IDX_SAVED };

static void submenu_cb(void *context, uint32_t index)
{
    IrApp *app = context;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_Start, index);

    switch(index) {
    case IDX_UNIVERSAL:
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_LIST, COLOR_DIM);
        view_popup_set_header(app->popup, "Coming Soon", COLOR_SECONDARY);
        view_popup_set_text(app->popup, "Universal remotes land in phase 2.");
        view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
        break;
    case IDX_LEARN:
        ir_remote_free(&app->current_remote);
        ir_remote_init(&app->current_remote, "");
        app->is_learning_new_remote = true;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Learn);
        break;
    case IDX_SAVED:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_RemoteList);
        break;
    }
}

void ir_scene_start_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "IR Remote", COLOR_ORANGE);

    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Universal Remotes",
                          COLOR_ORANGE, IDX_UNIVERSAL, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWNLOAD, "Learn New Remote",
                          COLOR_BLUE, IDX_LEARN, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SD_CARD, "Saved Remotes",
                          COLOR_CYAN, IDX_SAVED, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_Start));

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewSubmenu);
}

bool ir_scene_start_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    if(event.type == SceneEventTypeBack) {
        app_manager_exit_current();
        return true;
    }
    return false;
}

void ir_scene_start_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
