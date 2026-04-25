#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

void nfc_scene_nonces_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info, "Nonces", COLOR_MAGENTA);

    if(app->nonce_count == 0) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_LIST, COLOR_DIM);
        view_popup_set_header(app->popup, "No Nonces", COLOR_SECONDARY);
        view_popup_set_text(app->popup, "Emulate a card to capture nonces.");
        view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewPopup);
        return;
    }

    char hdr[64];
    snprintf(hdr, sizeof(hdr), "Captured: %u / %u", app->nonce_count, MFKEY_MAX_NONCES);
    view_info_add_field(app->info, "Status", hdr, COLOR_MAGENTA);
    view_info_add_separator(app->info);

    for(uint8_t i = 0; i < app->nonce_count; i++) {
        char buf[80];
        snprintf(buf, sizeof(buf), "S:%02u K:%c NT:%08lX",
                 app->nonces[i].sector,
                 app->nonces[i].key_type == 0 ? 'A' : 'B',
                 (unsigned long)app->nonces[i].nt);
        view_info_add_text_block(app->info, buf, FONT_MONO, COLOR_GREEN);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_nonces_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_nonces_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_info_reset(app->info);
    view_popup_reset(app->popup);
}
