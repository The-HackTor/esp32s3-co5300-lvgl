#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

/*
 * Capture Result -- shown after successful RAW capture.
 * Displays signal summary with two options:
 *   Retry → go back to Capture config
 *   More  → go to contextual Decoded menu (Save/Replay/Waveform)
 */

enum { SUBGHZ_EVT_RETRY = 0x400, SUBGHZ_EVT_MORE };

static void retry_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_RETRY);
}

static void more_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_MORE);
}

void subghz_scene_capture_result_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info, "Signal Captured", COLOR_CYAN);

    /* Frequency */
    char freq_buf[32];
    uint32_t mhz = app->frequency / 1000;
    uint32_t khz = app->frequency % 1000;
    lv_snprintf(freq_buf, sizeof(freq_buf), "%lu.%03lu MHz",
                (unsigned long)mhz, (unsigned long)khz);
    view_info_add_field(app->info, "Frequency", freq_buf, COLOR_YELLOW);
    view_info_add_separator(app->info);

    /* Sample count */
    char samples_buf[32];
    lv_snprintf(samples_buf, sizeof(samples_buf), "%u",
                (unsigned)app->raw.count);
    view_info_add_field(app->info, "Samples", samples_buf, COLOR_PRIMARY);
    view_info_add_separator(app->info);

    /* Decoded protocol if available */
    if(app->decoded_valid) {
        view_info_add_field(app->info, "Protocol",
                            app->last_decoded.protocol, COLOR_GREEN);
        view_info_add_separator(app->info);

        char data_buf[32];
        lv_snprintf(data_buf, sizeof(data_buf), "0x%lX",
                    (unsigned long)app->last_decoded.data);
        view_info_add_field(app->info, "Data", data_buf, COLOR_PRIMARY);
        view_info_add_separator(app->info);

        char bits_buf[16];
        lv_snprintf(bits_buf, sizeof(bits_buf), "%u bits",
                    (unsigned)app->last_decoded.bits);
        view_info_add_field(app->info, "Size", bits_buf, COLOR_SECONDARY);
        view_info_add_separator(app->info);
    }

    view_info_add_button_row(app->info,
        LV_SYMBOL_REFRESH " Retry", lv_color_hex(0x2A2A2A), retry_cb, app,
        "More " LV_SYMBOL_RIGHT,    COLOR_CYAN,              more_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewInfo);
}

bool subghz_scene_capture_result_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;
    if(event.type == SceneEventTypeCustom) {
        if(event.event == SUBGHZ_EVT_RETRY) {
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, subghz_SCENE_Capture);
            return true;
        }
        if(event.event == SUBGHZ_EVT_MORE) {
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Decoded);
            return true;
        }
    }
    return false;
}

void subghz_scene_capture_result_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_info_reset(app->info);
}
