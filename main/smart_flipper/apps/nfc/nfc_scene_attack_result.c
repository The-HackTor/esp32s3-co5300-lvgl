#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

void nfc_scene_attack_result_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info, "Recovered Keys", COLOR_GREEN);

    if (app->recovered_key_count == 0) {
        view_info_add_text_block(app->info, "No keys recovered.",
                                 FONT_BODY, COLOR_SECONDARY);
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d key(s) recovered", app->recovered_key_count);
        view_info_add_text_block(app->info, buf, FONT_BODY, COLOR_SECONDARY);

        for (int i = 0; i < app->recovered_key_count; i++) {
            const uint8_t *k = app->recovered_keys[i].key;
            char sector[16], hex[16];
            snprintf(sector, sizeof(sector), "S%d %c",
                     app->recovered_keys[i].sector,
                     app->recovered_keys[i].key_type == 0 ? 'A' : 'B');
            snprintf(hex, sizeof(hex), "%02X%02X%02X%02X%02X%02X",
                     k[0], k[1], k[2], k[3], k[4], k[5]);
            view_info_add_field(app->info, sector, hex, COLOR_CYAN);
        }
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_attack_result_on_event(void *ctx, SceneEvent event)
{
    NfcApp *app = ctx;

    if (event.type == SceneEventTypeBack) {
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, nfc_SCENE_Main);
        return true;
    }
    return false;
}

void nfc_scene_attack_result_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    view_info_reset(app->info);
}
