#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

static void submenu_cb(void *context, uint32_t index)
{
    SubghzApp *app = context;
    scene_manager_set_scene_state(&app->scene_mgr, subghz_SCENE_RelayMenu, index);

    switch(index) {
        case 0: scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Ping);     break;
        case 1: scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_SendDump); break;
        case 2: scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_RecvDump); break;
    }
}

void subghz_scene_relay_menu_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Relay", COLOR_MAGENTA);

    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, "Ping",
                          COLOR_GREEN, 0, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_UPLOAD, "Send Dump",
                          COLOR_MAGENTA, 1, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWNLOAD, "Receive Dump",
                          COLOR_MAGENTA, 2, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, subghz_SCENE_RelayMenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_relay_menu_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_relay_menu_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
}
