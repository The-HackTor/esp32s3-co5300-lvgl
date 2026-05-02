#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include "store/card_store.h"
#include <stdio.h>

static void card_selected(void *context, uint32_t index) {
    NfcApp *app = context;
    if(index < CARD_STORE_MAX_SLOTS && card_store_has((uint8_t)index)) {
        card_store_load((uint8_t)index, NULL, 0, &app->dump);
        app->dump_valid = true;
        app->card_type = NFC_CARD_MFC;
        app->selected_slot = (int)index;
        scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_SavedMenu);
    }
}

void nfc_scene_saved_list_on_enter(void *ctx) {
    NfcApp *app = ctx;
    bool any_saved = false;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Saved Cards", COLOR_BLUE);

    for(int i = 0; i < CARD_STORE_MAX_SLOTS; i++) {
        if(!card_store_has((uint8_t)i)) continue;
        any_saved = true;

        char name[32];
        if(card_store_load((uint8_t)i, name, sizeof(name), NULL) != 0) {
            snprintf(name, sizeof(name), "Card %d", i + 1);
        }
        view_submenu_add_item(app->submenu, LV_SYMBOL_SD_CARD, name,
                              COLOR_CYAN, (uint32_t)i, card_selected, app);
    }

    if(!any_saved) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_SD_CARD, COLOR_DIM);
        view_popup_set_header(app->popup, "No Saved Cards", COLOR_SECONDARY);
        view_popup_set_text(app->popup, "Read a card first, then save it here.");
        view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewPopup);
        return;
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, nfc_SCENE_SavedList));

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_saved_list_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_saved_list_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
