#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"

enum { IDX_SAVE, IDX_EMULATE, IDX_ATTACK, IDX_INFO };

static void submenu_cb(void *context, uint32_t index) {
    NfcApp *app = context;
    scene_manager_set_scene_state(&app->scene_mgr, nfc_SCENE_ReadMenu, index);

    switch(index) {
        case IDX_SAVE:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Save);
            break;
        case IDX_EMULATE:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Emulate);
            break;
        case IDX_ATTACK:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_NestedAttack);
            break;
        case IDX_INFO:
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_CardInfo);
            break;
    }
}

void nfc_scene_read_menu_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Card Read", COLOR_CYAN);

    view_submenu_add_item(app->submenu, LV_SYMBOL_SAVE, "Save",
                          COLOR_BLUE, IDX_SAVE, submenu_cb, app);

    view_submenu_add_item(app->submenu, LV_SYMBOL_COPY, "Emulate",
                          COLOR_ORANGE, IDX_EMULATE, submenu_cb, app);

    if (app->card_type != NFC_CARD_MFU) {
        int total_sectors = mfc_total_sectors(app->dump.type);
        int sectors_read = 0;
        for (int s = 0; s < total_sectors; s++) {
            int first = mfc_sector_first_block(s);
            int nblk = mfc_sector_block_count(s);
            for (int b = 0; b < nblk; b++) {
                if (app->dump.block_read[first + b]) { sectors_read++; break; }
            }
        }
        if (sectors_read < total_sectors) {
            view_submenu_add_item(app->submenu, LV_SYMBOL_CHARGE, "Attack",
                                  COLOR_MAGENTA, IDX_ATTACK, submenu_cb, app);
        }
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Info",
                          COLOR_CYAN, IDX_INFO, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, nfc_SCENE_ReadMenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_read_menu_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_read_menu_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_submenu_reset(app->submenu);
}
