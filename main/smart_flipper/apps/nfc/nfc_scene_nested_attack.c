#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <mf_classic_nested.h>
#include <mf_classic_hardnested.h>
#include <stdio.h>

static void nested_cb(bool success, const struct nested_result *res, void *ctx)
{
    NfcApp *app = ctx;

    if (success && app->recovered_key_count < MAX_RECOVERED_KEYS) {
        uint8_t sector = app->attack_target_sector;
        uint8_t key_type = app->attack_target_key_type;
        app->recovered_keys[app->recovered_key_count].sector = sector;
        app->recovered_keys[app->recovered_key_count].key_type =
            (key_type == MFC_KEY_A) ? 0 : 1;
        memcpy(app->recovered_keys[app->recovered_key_count].key,
               res->key, 6);
        app->recovered_key_count++;

        if (key_type == MFC_KEY_A) {
            memcpy(app->dump.key_a[sector], res->key, 6);
            app->dump.key_a_mask |= 1ULL << sector;
        } else {
            memcpy(app->dump.key_b[sector], res->key, 6);
            app->dump.key_b_mask |= 1ULL << sector;
        }
    }

    if (res && res->prng_type == NESTED_PRNG_HARD) {
        scene_manager_handle_custom_event(&app->scene_mgr,
                                          NFC_EVT_NESTED_FAIL);
        return;
    }

    scene_manager_handle_custom_event(
        &app->scene_mgr,
        success ? NFC_EVT_NESTED_DONE : NFC_EVT_NESTED_FAIL);
}

static void hardnested_cb(bool success, const struct hardnested_result *res,
                          void *ctx)
{
    NfcApp *app = ctx;

    if (success && app->recovered_key_count < MAX_RECOVERED_KEYS) {
        uint8_t sector = app->attack_target_sector;
        uint8_t key_type = app->attack_target_key_type;
        app->recovered_keys[app->recovered_key_count].sector = sector;
        app->recovered_keys[app->recovered_key_count].key_type =
            (key_type == MFC_KEY_A) ? 0 : 1;
        memcpy(app->recovered_keys[app->recovered_key_count].key,
               res->key, 6);
        app->recovered_key_count++;

        if (key_type == MFC_KEY_A) {
            memcpy(app->dump.key_a[sector], res->key, 6);
            app->dump.key_a_mask |= 1ULL << sector;
        } else {
            memcpy(app->dump.key_b[sector], res->key, 6);
            app->dump.key_b_mask |= 1ULL << sector;
        }
    }

    scene_manager_handle_custom_event(
        &app->scene_mgr,
        success ? NFC_EVT_HARDNESTED_DONE : NFC_EVT_HARDNESTED_FAIL);
}

static bool find_known_key(NfcApp *app, struct nested_params *params)
{
    int total = mfc_total_sectors(app->dump.type);

    for (int s = 0; s < total; s++) {
        if (mfc_key_a_found(&app->dump, s)) {
            params->known_sector = s;
            params->known_key_type = MFC_KEY_A;
            memcpy(params->known_key, app->dump.key_a[s], 6);
            return true;
        }
        if (mfc_key_b_found(&app->dump, s)) {
            params->known_sector = s;
            params->known_key_type = MFC_KEY_B;
            memcpy(params->known_key, app->dump.key_b[s], 6);
            return true;
        }
    }

    if (app->recovered_key_count > 0) {
        params->known_sector = app->recovered_keys[0].sector;
        params->known_key_type = (app->recovered_keys[0].key_type == 0)
                                 ? MFC_KEY_A : MFC_KEY_B;
        memcpy(params->known_key, app->recovered_keys[0].key, 6);
        return true;
    }

    return false;
}

static bool find_target_sector(NfcApp *app, struct nested_params *params)
{
    int total = mfc_total_sectors(app->dump.type);

    for (int s = 0; s < total; s++) {
        if (!mfc_key_a_found(&app->dump, s)) {
            params->target_sector = s;
            params->target_key_type = MFC_KEY_A;
            return true;
        }
        if (!mfc_key_b_found(&app->dump, s)) {
            params->target_sector = s;
            params->target_key_type = MFC_KEY_B;
            return true;
        }
    }
    return false;
}

void nfc_scene_nested_attack_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    app->recovered_key_count = 0;

    if (app->card_type == NFC_CARD_MFU) {
        scene_manager_previous_scene(&app->scene_mgr);
        return;
    }

    view_action_reset(app->action);
    view_action_set_header(app->action, "Nested Attack", COLOR_ORANGE);
    view_action_set_header_detail(app->action, "Analyzing PRNG...");
    view_action_set_text(app->action, LV_SYMBOL_CHARGE);
    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewAction);

    struct nested_params params;
    memset(&params, 0, sizeof(params));
    params.uid = bytes_to_be32(app->dump.card.uid);

    if (!find_known_key(app, &params) || !find_target_sector(app, &params)) {
        view_action_set_header_detail(app->action, "No known key found");
        view_action_set_text(app->action, LV_SYMBOL_WARNING);
        scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_NESTED_FAIL);
        return;
    }

    app->attack_target_sector = params.target_sector;
    app->attack_target_key_type = params.target_key_type;

    hw_nfc_start_nested(&params, nested_cb, app);
}

bool nfc_scene_nested_attack_on_event(void *ctx, SceneEvent event)
{
    NfcApp *app = ctx;

    if (event.type == SceneEventTypeCustom) {
        if (event.event == NFC_EVT_NESTED_DONE ||
            event.event == NFC_EVT_HARDNESTED_DONE) {
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_AttackResult);
            return true;
        }
        if (event.event == NFC_EVT_NESTED_FAIL) {
            view_action_set_header(app->action, "Hardnested Attack", COLOR_MAGENTA);
            view_action_set_header_detail(app->action, "Hard PRNG, collecting nonces...");
            view_action_set_text(app->action, LV_SYMBOL_CHARGE);

            struct nested_params params;
            memset(&params, 0, sizeof(params));
            params.uid = bytes_to_be32(app->dump.card.uid);

            if (!find_known_key(app, &params) ||
                !find_target_sector(app, &params)) {
                scene_manager_handle_custom_event(&app->scene_mgr,
                                                   NFC_EVT_HARDNESTED_FAIL);
                return true;
            }

            app->attack_target_sector = params.target_sector;
            app->attack_target_key_type = params.target_key_type;

            hw_nfc_start_hardnested(&params, hardnested_cb, app);
            return true;
        }
        if (event.event == NFC_EVT_HARDNESTED_FAIL) {
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

void nfc_scene_nested_attack_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    view_action_reset(app->action);
}
