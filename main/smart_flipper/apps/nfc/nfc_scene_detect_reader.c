#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>
#include <string.h>

/*
 * Detect Reader -- emulate a fake MIFARE Classic card and capture
 * authentication nonces from a reader. Nonces are paired (2 per sector)
 * for mfkey32 key recovery.
 *
 * Flow: Waiting → Reader Detected (collecting) → Done
 */

static void nonce_cb(const struct mfkey_nonce *nonce, void *ctx)
{
    NfcApp *app = ctx;

    if(app->nonce_count < MFKEY_MAX_NONCES) {
        app->nonces[app->nonce_count] = *nonce;
    }
    app->nonce_count++;

    /* mfkey32v2 requires two auths against the SAME (sector, key_type).
     * Fill an open slot if one exists for this sector+key; otherwise open
     * a new one. Ignores drops once MFKEY_MAX_PAIRS slots are all filled. */
    for(int p = 0; p < app->nonce_pair_count; p++) {
        if(app->nonce_pairs[p].complete) continue;
        if(app->nonce_pairs[p].sector != nonce->sector) continue;
        if(app->nonce_pairs[p].key_type != nonce->key_type) continue;

        app->nonce_pairs[p].nt1 = nonce->nt;
        app->nonce_pairs[p].nr1 = nonce->nr;
        app->nonce_pairs[p].ar1 = nonce->ar;
        app->nonce_pairs[p].complete = true;
        scene_manager_handle_custom_event(&app->scene_mgr,
                                          NFC_EVT_NONCE_PAIR_COMPLETE);
        scene_manager_handle_custom_event(&app->scene_mgr,
                                          NFC_EVT_NONCE_CAPTURED);
        return;
    }

    if(app->nonce_pair_count < MFKEY_MAX_PAIRS) {
        int p = app->nonce_pair_count++;
        app->nonce_pairs[p].sector   = nonce->sector;
        app->nonce_pairs[p].key_type = nonce->key_type;
        app->nonce_pairs[p].complete = false;
        app->nonce_pairs[p].nt0 = nonce->nt;
        app->nonce_pairs[p].nr0 = nonce->nr;
        app->nonce_pairs[p].ar0 = nonce->ar;
    }

    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_NONCE_CAPTURED);
}

static void stop_cb(void *ctx)
{
    NfcApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_STOP_EMULATE);
}

void nfc_scene_detect_reader_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    /* Reset nonce pair state */
    app->nonce_pair_count = 0;
    app->nonce_count = 0;
    memset(app->nonce_pairs, 0, sizeof(app->nonce_pairs));

    /* Generate a fake card for emulation */
    hw_nfc_generate_fake_dump(&app->detect_dump);

    view_action_reset(app->action);
    view_action_set_header(app->action, "Extract Keys", COLOR_ORANGE);
    view_action_set_header_detail(app->action, "Waiting for reader...");
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "Pairs  0 / 10", COLOR_SECONDARY);
    view_action_show_arc_progress(app->action, true, COLOR_ORANGE);
    view_action_set_cancel_button(app->action, LV_SYMBOL_STOP " Stop",
                                  COLOR_RED, stop_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewAction);

    app->emulating = true;
    hw_nfc_start_emulate(&app->detect_dump, nonce_cb, app);
}

bool nfc_scene_detect_reader_on_event(void *ctx, SceneEvent event)
{
    NfcApp *app = ctx;
    if(event.type != SceneEventTypeCustom) return false;

    if(event.event == NFC_EVT_NONCE_CAPTURED) {
        char buf[32];
        int complete = 0;
        for(int i = 0; i < app->nonce_pair_count; i++) {
            if(app->nonce_pairs[i].complete) complete++;
        }
        view_action_set_header_detail(app->action, "Collecting...");
        snprintf(buf, sizeof(buf), "Pairs  %d / %d", complete, MFKEY_MAX_PAIRS);
        view_action_set_detail(app->action, buf, COLOR_CYAN);
        return true;
    }

    if(event.event == NFC_EVT_NONCE_PAIR_COMPLETE) {
        int complete = 0;
        for(int i = 0; i < app->nonce_pair_count; i++) {
            if(app->nonce_pairs[i].complete) complete++;
        }
        if(complete >= MFKEY_MAX_PAIRS) {
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_MfkeySolve);
        }
        return true;
    }

    if(event.event == NFC_EVT_STOP_EMULATE) {
        int complete = 0;
        for(int i = 0; i < app->nonce_pair_count; i++) {
            if(app->nonce_pairs[i].complete) complete++;
        }
        if(complete > 0) {
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_MfkeyNonces);
        } else {
            scene_manager_previous_scene(&app->scene_mgr);
        }
        return true;
    }

    return false;
}

void nfc_scene_detect_reader_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    hw_nfc_stop_emulate();
    app->emulating = false;
    view_action_reset(app->action);
}
