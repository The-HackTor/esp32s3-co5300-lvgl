#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

/*
 * MFKey Nonces Info -- shows captured nonce pairs with sector details.
 * User presses OK to continue to the completion screen.
 */

static void done_cb(void *ctx)
{
    NfcApp *app = ctx;
    scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_MfkeySolve);
}

void nfc_scene_mfkey_nonces_on_enter(void *ctx)
{
    NfcApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info, "Nonces", COLOR_ORANGE);

    int complete = 0;
    for(int i = 0; i < app->nonce_pair_count; i++) {
        if(app->nonce_pairs[i].complete) complete++;
    }

    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%d", complete);
    view_info_add_field(app->info, "Pairs saved", hdr, COLOR_GREEN);

    view_info_add_separator(app->info);

    for(int i = 0; i < app->nonce_pair_count; i++) {
        if(!app->nonce_pairs[i].complete) continue;

        char buf[32];
        snprintf(buf, sizeof(buf), "Sector %u, Key %c",
                 app->nonce_pairs[i].sector,
                 app->nonce_pairs[i].key_type == 0 ? 'A' : 'B');
        view_info_add_text_block(app->info, buf, FONT_BODY, COLOR_CYAN);
    }

    view_info_add_separator(app->info);

    view_info_add_button(app->info, LV_SYMBOL_OK " Continue",
                         COLOR_CYAN, done_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_mfkey_nonces_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_mfkey_nonces_on_exit(void *ctx)
{
    NfcApp *app = ctx;
    view_info_reset(app->info);
}
