#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

static void emulate_cb(const struct mfkey_nonce *nonce, void *ctx) {
    NfcApp *app = ctx;
    if(app->nonce_count < MFKEY_MAX_NONCES) {
        app->nonces[app->nonce_count] = *nonce;
        app->nonce_count++;
    }
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_NONCE_CAPTURED);
}

static void stop_cb(void *ctx) {
    NfcApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_STOP_EMULATE);
}

void nfc_scene_emulate_on_enter(void *ctx) {
    NfcApp *app = ctx;
    char uid_buf[32];

    view_action_reset(app->action);
    view_action_set_header(app->action, "Emulating", COLOR_ORANGE);

    snprintf(uid_buf, sizeof(uid_buf), "%s\nUID: %02X %02X %02X %02X",
             mfc_type_str(app->dump.type),
             app->dump.card.uid[0], app->dump.card.uid[1],
             app->dump.card.uid[2], app->dump.card.uid[3]);
    view_action_set_header_detail(app->action, uid_buf);
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "Nonces: 0", COLOR_SECONDARY);
    view_action_show_arc_progress(app->action, true, COLOR_ORANGE);
    view_action_set_cancel_button(app->action, LV_SYMBOL_CLOSE " Stop",
                                  COLOR_RED, stop_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewAction);

    if (app->card_type == NFC_CARD_MFU) {
        const uint8_t *u = app->mfu_dump.card.uid;
        snprintf(uid_buf, sizeof(uid_buf),
                 "%s\nUID: %02X %02X %02X %02X %02X %02X %02X",
                 mfu_type_str(app->mfu_dump.type),
                 u[0], u[1], u[2], u[3], u[4], u[5], u[6]);
        view_action_set_header_detail(app->action, uid_buf);
        view_action_set_detail(app->action, "Present to reader",
                               COLOR_SECONDARY);

        app->emulating = true;
        hw_nfc_start_emulate_mfu(&app->mfu_dump);
        return;
    }

    app->emulating = true;
    hw_nfc_start_emulate(&app->dump, emulate_cb, app);
}

bool nfc_scene_emulate_on_event(void *ctx, SceneEvent event) {
    NfcApp *app = ctx;
    if(event.type == SceneEventTypeCustom) {
        if(event.event == NFC_EVT_NONCE_CAPTURED) {
            char nbuf[48];
            snprintf(nbuf, sizeof(nbuf), "Nonces: %u", app->nonce_count);
            view_action_set_detail(app->action, nbuf, COLOR_SECONDARY);
            return true;
        }
        if(event.event == NFC_EVT_STOP_EMULATE) {
            scene_manager_previous_scene(&app->scene_mgr);
            return true;
        }
    }
    return false;
}

void nfc_scene_emulate_on_exit(void *ctx) {
    NfcApp *app = ctx;
    hw_nfc_stop_emulate();
    app->emulating = false;
    view_action_reset(app->action);
}
