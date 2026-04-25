#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include "store/card_store.h"
#include <stdio.h>

static void submenu_cb(void *context, uint32_t index) {
    NfcApp *app = context;
    if(index < CARD_STORE_MAX_SLOTS) {
        if (app->card_type == NFC_CARD_MFU) {
            scene_manager_previous_scene(&app->scene_mgr);
            return;
        }
        char name[32];
        snprintf(name, sizeof(name), "Card %u", index + 1);
        card_store_save((uint8_t)index, name, &app->dump);

        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, nfc_SCENE_Main);
    }
}

void nfc_scene_save_on_enter(void *ctx) {
    NfcApp *app = ctx;
    char buf[48];

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Save Card", COLOR_CYAN);

    for(int i = 0; i < CARD_STORE_MAX_SLOTS; i++) {
        if(card_store_has((uint8_t)i)) {
            char name[32];
            if(card_store_load((uint8_t)i, name, sizeof(name), NULL) == 0) {
                snprintf(buf, sizeof(buf), "%s (used)", name);
            } else {
                snprintf(buf, sizeof(buf), "Slot %d (used)", i + 1);
            }
        } else {
            snprintf(buf, sizeof(buf), "Card %d", i + 1);
        }
        view_submenu_add_item(app->submenu, LV_SYMBOL_SAVE, buf,
                              COLOR_BLUE, (uint32_t)i, submenu_cb, app);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_save_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_save_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_submenu_reset(app->submenu);
}
