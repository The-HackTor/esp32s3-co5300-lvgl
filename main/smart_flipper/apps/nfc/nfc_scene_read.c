#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"

static void cancel_cb(void *ctx) {
    NfcApp *app = ctx;
    hw_nfc_cancel_read();
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_READ_FAIL);
}

static void read_cb(bool success, enum nfc_card_type type, void *ctx) {
    NfcApp *app = ctx;
    if(success) {
        app->card_type = type;
        scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_READ_DONE);
    } else {
        scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_READ_FAIL);
    }
}

void nfc_scene_read_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_action_reset(app->action);
    view_action_set_header(app->action, "Reading...", COLOR_CYAN);
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "Place card near device", COLOR_SECONDARY);
    view_action_show_spinner(app->action, true, COLOR_CYAN);
    view_action_set_cancel_button(app->action, LV_SYMBOL_CLOSE " Cancel",
                                  COLOR_RED, cancel_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewAction);

    hw_nfc_start_read(&app->dump, &app->mfu_dump, read_cb, app);
}

bool nfc_scene_read_on_event(void *ctx, SceneEvent event) {
    NfcApp *app = ctx;
    if(event.type == SceneEventTypeCustom) {
        if(event.event == NFC_EVT_READ_DONE) {
            app->dump_valid = true;
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_ReadResult);
            return true;
        }
        if(event.event == NFC_EVT_READ_FAIL) {
            scene_manager_previous_scene(&app->scene_mgr);
            return true;
        }
    }
    return false;
}

void nfc_scene_read_on_exit(void *ctx) {
    NfcApp *app = ctx;
    hw_nfc_cancel_read();
    view_action_reset(app->action);
}
