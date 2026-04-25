#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

/*
 * MFKey Complete -- informs user that nonce collection is done
 * and keys can be extracted using mfkey32.
 */

static void finish_cb(void *ctx)
{
    NfcApp *app = ctx;
    /* Go all the way back to main menu */
    scene_manager_search_and_switch_to_previous_scene(
        &app->scene_mgr, nfc_SCENE_Main);
}

void nfc_scene_mfkey_complete_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    /* Count completed pairs */
    int complete = 0;
    for(int i = 0; i < app->nonce_pair_count; i++) {
        if(app->nonce_pairs[i].nt1 != 0) complete++;
    }

    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
    view_popup_set_header(app->popup, "Complete!", COLOR_GREEN);

    char buf[64];
    snprintf(buf, sizeof(buf), "%d nonce pairs captured.\n"
             "Run MFKey32 to extract keys.", complete);
    view_popup_set_text(app->popup, buf);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewPopup);

    /* Auto-return after 4 seconds */
    view_popup_set_timeout(app->popup, 4000, (ViewPopupTimeoutCb)finish_cb, app);
}

bool nfc_scene_mfkey_complete_on_event(void *ctx, SceneEvent event)
{
    NfcApp *app = ctx;
    if(event.type == SceneEventTypeBack) {
        finish_cb(app);
        return true;
    }
    return false;
}

void nfc_scene_mfkey_complete_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    view_popup_reset(app->popup);
}
