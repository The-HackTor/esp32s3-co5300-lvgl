#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "apps/nfc/nfc_app.h"
#include "hw/hw_subghz.h"
#include <string.h>

static int last_progress;

static void recv_cb(bool done, int progress, void *ctx)
{
    SubghzApp *app = ctx;
    last_progress = progress;
    if(done)
        scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_RELAY_DONE);
    else
        scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_RELAY_PROGRESS);
}

static void stop_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
}

void subghz_scene_recv_dump_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    last_progress = 0;

    view_action_reset(app->action);
    view_action_set_header(app->action, "Receive Dump", COLOR_MAGENTA);
    view_action_set_header_detail(app->action, "Listening...");
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "0%", COLOR_SECONDARY);
    view_action_show_arc_progress(app->action, true, COLOR_MAGENTA);
    view_action_set_progress(app->action, 0);
    view_action_set_cancel_button(app->action, LV_SYMBOL_STOP " Stop",
                                  COLOR_RED, stop_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewAction);

    sim_subghz_recv_dump(recv_cb, app);
}

bool subghz_scene_recv_dump_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;
    if(event.type != SceneEventTypeCustom) return false;

    if(event.event == SUBGHZ_EVT_RELAY_PROGRESS) {
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "Receiving  %d%%", last_progress);
        view_action_set_header_detail(app->action, buf);
        view_action_set_progress(app->action, (int32_t)(last_progress * 360 / 100));
        return true;
    }
    if(event.event == SUBGHZ_EVT_RELAY_DONE) {
        NfcApp *nfc = nfc_app_get();
        memcpy(&nfc->dump, hw_subghz_get_relay_dump(), sizeof(nfc->dump));
        nfc->dump_valid = true;

        view_action_show_arc_progress(app->action, false, COLOR_GREEN);
        view_action_set_header_detail(app->action, "");
        view_action_set_text(app->action, LV_SYMBOL_OK);
        view_action_set_detail(app->action, "NFC card loaded", COLOR_GREEN);
        view_action_set_cancel_button(app->action, LV_SYMBOL_OK " Done",
                                      COLOR_GREEN, stop_cb, app);
        return true;
    }
    return false;
}

void subghz_scene_recv_dump_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    sim_subghz_stop_relay();
    view_action_reset(app->action);
}
