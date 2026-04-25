#include "nfc_app.h"
#include "nfc_scenes.h"
#include "app/app_manager.h"
#include "ui/styles.h"

enum { IDX_READ, IDX_EXTRACT_KEYS, IDX_SAVED, IDX_PROTOCOLS };

static void submenu_cb(void *context, uint32_t index) {
    NfcApp *app = context;
    scene_manager_set_scene_state(&app->scene_mgr, nfc_SCENE_Main, index);

    switch(index) {
        case IDX_READ:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Read);
            break;
        case IDX_EXTRACT_KEYS:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_DetectReader);
            break;
        case IDX_SAVED:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_SavedList);
            break;
        case IDX_PROTOCOLS:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Protocols);
            break;
    }
}

void nfc_scene_main_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "NFC", COLOR_CYAN);

    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWNLOAD, "Read",
                          COLOR_CYAN, IDX_READ, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SHUFFLE, "Extract Keys",
                          COLOR_ORANGE, IDX_EXTRACT_KEYS, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SD_CARD, "Saved",
                          COLOR_BLUE, IDX_SAVED, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Protocols",
                          COLOR_ORANGE, IDX_PROTOCOLS, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, nfc_SCENE_Main));

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_main_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    if(event.type == SceneEventTypeBack) {
        app_manager_exit_current();
        return true;
    }
    return false;
}

void nfc_scene_main_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_submenu_reset(app->submenu);
}
