#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

static void tx_done_cb(bool success, void *ctx)
{
    (void)success;
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_ENCODE_TX_DONE);
}

void subghz_scene_encode_tx_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_action_reset(app->action);
    view_action_set_header(app->action, "Transmitting", COLOR_ORANGE);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s  0x%08lX",
             app->last_decoded.protocol ? app->last_decoded.protocol : "?",
             (unsigned long)(app->last_decoded.data & 0xFFFFFFFF));
    view_action_set_header_detail(app->action, buf);
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "Sending via encoder...", COLOR_YELLOW);
    view_action_show_arc_progress(app->action, true, COLOR_ORANGE);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewAction);

    hw_subghz_start_encode_tx(
        app->last_decoded.protocol ? app->last_decoded.protocol : "Princeton",
        app->last_decoded.data,
        app->last_decoded.bits,
        app->last_decoded.te,
        5,
        tx_done_cb, app);
}

bool subghz_scene_encode_tx_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if (event.type == SceneEventTypeCustom &&
        event.event == SUBGHZ_EVT_ENCODE_TX_DONE) {
        view_action_set_header(app->action, "Sent!", COLOR_GREEN);
        view_action_set_header_detail(app->action, "");
        view_action_set_text(app->action, LV_SYMBOL_OK);
        view_action_set_detail(app->action, "Transmission complete", COLOR_GREEN);
        view_action_show_arc_progress(app->action, false, COLOR_GREEN);
        return true;
    }
    if (event.type == SceneEventTypeBack) {
        scene_manager_previous_scene(&app->scene_mgr);
        return true;
    }
    return false;
}

void subghz_scene_encode_tx_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_action_reset(app->action);
}
