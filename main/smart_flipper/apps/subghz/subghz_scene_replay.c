#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

static bool using_encoder;
static bool transmitting;

static SubghzApp *replay_app_ctx;
static void replay_cb(bool success, void *ctx);

static void replay_restart_timer_cb(lv_timer_t *t)
{
    lv_timer_delete(t);
    SubghzApp *app = replay_app_ctx;
    if(!app || !transmitting) return;

    if(using_encoder) {
        hw_subghz_start_encode_tx(
            app->last_decoded.protocol,
            app->last_decoded.data,
            app->last_decoded.bits,
            app->last_decoded.te,
            10,
            replay_cb, app);
    } else {
        sim_subghz_start_replay(&app->raw, replay_cb, app);
    }
}

static void replay_cb(bool success, void *ctx)
{
    (void)success;
    SubghzApp *app = ctx;
    if(transmitting && using_encoder) {
        replay_app_ctx = app;
        lv_timer_create(replay_restart_timer_cb, 10, NULL);
    } else {
        transmitting = false;
        scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_REPLAY_DONE);
    }
}

static void send_again_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
    scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Replay);
}

static void stop_cb(void *ctx)
{
    SubghzApp *app = ctx;
    transmitting = false;
    hw_subghz_stop_replay();
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_REPLAY_DONE);
}

void subghz_scene_replay_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_action_reset(app->action);

    if(!app->raw_valid && !app->decoded_valid) {
        view_action_set_header(app->action, "Replay", COLOR_ORANGE);
        view_action_set_header_detail(app->action, "No signal captured");
        view_action_set_text(app->action, LV_SYMBOL_WARNING);
        view_action_set_detail(app->action, "Capture a signal first", COLOR_RED);
        view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewAction);
        return;
    }

    char buf[64];
    uint32_t mhz = app->frequency / 1000;
    uint32_t khz = app->frequency % 1000;

    using_encoder = app->decoded_valid && app->last_decoded.protocol;

    if(using_encoder) {
        view_action_set_header(app->action, "Transmitting", COLOR_GREEN);
        lv_snprintf(buf, sizeof(buf), "%s  0x%08lX\n%lu.%03lu MHz",
                    app->last_decoded.protocol,
                    (unsigned long)(app->last_decoded.data & 0xFFFFFFFF),
                    (unsigned long)mhz, (unsigned long)khz);
    } else {
        view_action_set_header(app->action, "Transmitting RAW", COLOR_ORANGE);
        lv_snprintf(buf, sizeof(buf), "%u samples\n%lu.%03lu MHz",
                    (unsigned)app->raw.count,
                    (unsigned long)mhz, (unsigned long)khz);
    }

    view_action_set_text(app->action, LV_SYMBOL_WIFI);
    view_action_set_detail(app->action, buf, COLOR_SECONDARY);
    view_action_show_arc_progress(app->action, true,
                                  using_encoder ? COLOR_GREEN : COLOR_ORANGE);
    view_action_set_cancel_button(app->action,
                                  LV_SYMBOL_STOP " Stop",
                                  COLOR_RED, stop_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewAction);

    transmitting = true;

    if(using_encoder) {
        hw_subghz_start_encode_tx(
            app->last_decoded.protocol,
            app->last_decoded.data,
            app->last_decoded.bits,
            app->last_decoded.te,
            10,
            replay_cb, app);
    } else {
        sim_subghz_start_replay(&app->raw, replay_cb, app);
    }
}

bool subghz_scene_replay_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if(event.type == SceneEventTypeCustom) {
        if(event.event == SUBGHZ_EVT_REPLAY_DONE ||
           event.event == SUBGHZ_EVT_ENCODE_TX_DONE) {
            transmitting = false;
            view_action_show_arc_progress(app->action, false, COLOR_GREEN);
            view_action_set_header(app->action, "Stopped", COLOR_GREEN);
            view_action_set_text(app->action, LV_SYMBOL_OK);
            view_action_set_detail(app->action, "Transmission stopped",
                                   COLOR_GREEN);
            view_action_set_cancel_button(app->action,
                                          LV_SYMBOL_REFRESH " Send again",
                                          COLOR_ORANGE, send_again_cb, app);
            return true;
        }
    }
    return false;
}

void subghz_scene_replay_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    transmitting = false;
    hw_subghz_stop_replay();
    view_action_reset(app->action);
    using_encoder = false;
}
