#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <mf_classic_mfkey32.h>
#include <stdio.h>

static void mfkey_solve_cb(const struct mfkey32_result *results,
                           size_t count, void *ctx)
{
    NfcApp *app = ctx;
    int found = 0;

    for (size_t i = 0; i < count; i++) {
        if (results[i].found && app->recovered_key_count < MAX_RECOVERED_KEYS) {
            app->recovered_keys[app->recovered_key_count].sector =
                app->nonce_pairs[i].sector;
            app->recovered_keys[app->recovered_key_count].key_type =
                app->nonce_pairs[i].key_type;
            memcpy(app->recovered_keys[app->recovered_key_count].key,
                   results[i].key, 6);
            app->recovered_key_count++;
            found++;
        }
    }

    scene_manager_handle_custom_event(
        &app->scene_mgr,
        found > 0 ? NFC_EVT_MFKEY_SOLVE_DONE : NFC_EVT_MFKEY_SOLVE_FAIL);
}

void nfc_scene_mfkey_solve_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    app->recovered_key_count = 0;

    view_action_reset(app->action);
    view_action_set_header(app->action, "Solving Keys...", COLOR_CYAN);
    view_action_set_header_detail(app->action, "Running MFKey32 solver");
    view_action_set_text(app->action, LV_SYMBOL_CHARGE);
    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewAction);

    int pair_count = 0;
    struct mfkey32_nonce_pair pairs[MFKEY_MAX_PAIRS];

    const uint8_t *uid = app->detect_dump.card.uid_len > 0
                         ? app->detect_dump.card.uid
                         : app->dump.card.uid;

    for (int i = 0; i < app->nonce_pair_count; i++) {
        if (!app->nonce_pairs[i].complete)
            continue;
        pairs[pair_count].uid = (uint32_t)uid[0] << 24 |
                                (uint32_t)uid[1] << 16 |
                                (uint32_t)uid[2] << 8 |
                                (uint32_t)uid[3];
        pairs[pair_count].nt0 = app->nonce_pairs[i].nt0;
        pairs[pair_count].nr0 = app->nonce_pairs[i].nr0;
        pairs[pair_count].ar0 = app->nonce_pairs[i].ar0;
        pairs[pair_count].nt1 = app->nonce_pairs[i].nt1;
        pairs[pair_count].nr1 = app->nonce_pairs[i].nr1;
        pairs[pair_count].ar1 = app->nonce_pairs[i].ar1;
        pair_count++;
    }

    if (pair_count > 0) {
        hw_nfc_solve_mfkey32(pairs, pair_count, mfkey_solve_cb, app);
    } else {
        scene_manager_handle_custom_event(&app->scene_mgr,
                                          NFC_EVT_MFKEY_SOLVE_FAIL);
    }
}

bool nfc_scene_mfkey_solve_on_event(void *ctx, SceneEvent event)
{
    NfcApp *app = ctx;

    if (event.type == SceneEventTypeCustom) {
        if (event.event == NFC_EVT_MFKEY_SOLVE_DONE) {
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_AttackResult);
            return true;
        }
        if (event.event == NFC_EVT_MFKEY_SOLVE_FAIL) {
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, nfc_SCENE_Main);
            return true;
        }
    }
    if (event.type == SceneEventTypeBack) {
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, nfc_SCENE_Main);
        return true;
    }
    return false;
}

void nfc_scene_mfkey_solve_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    view_action_reset(app->action);
}
