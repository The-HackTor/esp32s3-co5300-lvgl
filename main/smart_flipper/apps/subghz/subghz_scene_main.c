#include "subghz_app.h"
#include "subghz_scenes.h"
#include "app/app_manager.h"
#include "ui/styles.h"

/*
 * SubGHz Main Menu -- Flipper Zero pattern:
 *   Read          → live signal receiver
 *   Read RAW      → raw capture
 *   Saved         → pick saved signal → contextual menu
 *   Freq Analyzer → frequency scanner
 *   Relay         → relay submenu
 *   Settings      → frequency/preset config
 *
 * "Replay", "Waveform", "Signal Detail" are NOT here --
 * they appear contextually after capture or from saved signals.
 */

enum {
    IDX_READ,
    IDX_READ_RAW,
    IDX_SAVED,
    IDX_ANALYZER,
    IDX_RELAY,
    IDX_SETTINGS,
};

static void submenu_cb(void *context, uint32_t index)
{
    SubghzApp *app = context;
    scene_manager_set_scene_state(&app->scene_mgr, subghz_SCENE_Main, index);

    switch(index) {
        case IDX_READ:      scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Read);      break;
        case IDX_READ_RAW:  scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Capture);   break;
        case IDX_SAVED:     scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_SavedList); break;
        case IDX_ANALYZER:  scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Analyzer);  break;
        case IDX_RELAY:     scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_RelayMenu); break;
        case IDX_SETTINGS:  scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Settings);  break;
    }
}

void subghz_scene_main_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "SubGHz", COLOR_ORANGE);

    view_submenu_add_item(app->submenu, LV_SYMBOL_EYE_OPEN, "Read",
                          COLOR_GREEN, IDX_READ, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWNLOAD, "Read RAW",
                          COLOR_CYAN, IDX_READ_RAW, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SD_CARD, "Saved",
                          COLOR_BLUE, IDX_SAVED, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_LOOP, "Analyzer",
                          COLOR_YELLOW, IDX_ANALYZER, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Relay",
                          COLOR_MAGENTA, IDX_RELAY, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SETTINGS, "Settings",
                          COLOR_SECONDARY, IDX_SETTINGS, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, subghz_SCENE_Main));

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_main_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    if(event.type == SceneEventTypeBack) {
        app_manager_exit_current();
        return true;
    }
    return false;
}

void subghz_scene_main_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
}
