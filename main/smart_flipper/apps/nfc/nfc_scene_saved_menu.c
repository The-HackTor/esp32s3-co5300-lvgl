#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include "store/card_store.h"

/*
 * Saved Menu -- contextual menu for a selected saved card.
 * Like Flipper Zero: Emulate, Info, Delete
 */

enum { IDX_EMULATE, IDX_INFO, IDX_DELETE };

static void submenu_cb(void *context, uint32_t index) {
    NfcApp *app = context;

    switch(index) {
        case IDX_EMULATE:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Emulate);
            break;
        case IDX_INFO:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_CardInfo);
            break;
        case IDX_DELETE:
            if(app->selected_slot >= 0) {
                card_store_delete((uint8_t)app->selected_slot);
                app->dump_valid = false;
            }
            /* Go back to saved list */
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, nfc_SCENE_SavedList);
            break;
    }
}

void nfc_scene_saved_menu_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Saved Card", COLOR_BLUE);

    view_submenu_add_item(app->submenu, LV_SYMBOL_COPY, "Emulate",
                          COLOR_ORANGE, IDX_EMULATE, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Info",
                          COLOR_CYAN, IDX_INFO, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Delete",
                          COLOR_RED, IDX_DELETE, submenu_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_saved_menu_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_saved_menu_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_submenu_reset(app->submenu);
}
