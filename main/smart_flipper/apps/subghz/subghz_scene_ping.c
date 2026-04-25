#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

static void ping_cb(bool success, void *ctx)
{
    (void)success;
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_PING_DONE);
}

void subghz_scene_ping_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_action_reset(app->action);
    view_action_set_header(app->action, "Ping", COLOR_GREEN);
    view_action_set_header_detail(app->action, "Pinging...");
    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, "Testing SubGHz connection", COLOR_SECONDARY);
    view_action_show_spinner(app->action, true, COLOR_GREEN);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewAction);

    sim_subghz_ping(ping_cb, app);
}

bool subghz_scene_ping_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if(event.type == SceneEventTypeCustom && event.event == SUBGHZ_EVT_PING_DONE) {
        view_action_show_spinner(app->action, false, COLOR_GREEN);
        view_action_set_header_detail(app->action, "");
        view_action_set_text(app->action, LV_SYMBOL_OK);
        view_action_set_detail(app->action, "Pong received!", COLOR_GREEN);
        return true;
    }
    return false;
}

void subghz_scene_ping_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_action_reset(app->action);
}
