#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

enum {
    ReceiverInfoEvtSend,
    ReceiverInfoEvtSave,
};

static void send_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, ReceiverInfoEvtSend);
}

static void save_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, ReceiverInfoEvtSave);
}

void subghz_scene_receiver_info_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info,
                         app->last_decoded.protocol
                             ? app->last_decoded.protocol
                             : "Signal",
                         COLOR_GREEN);

    char freq_buf[32];
    uint32_t mhz = app->frequency / 1000;
    uint32_t khz = app->frequency % 1000;
    lv_snprintf(freq_buf, sizeof(freq_buf), "%lu.%03lu MHz",
                (unsigned long)mhz, (unsigned long)khz);
    view_info_add_field(app->info, "Frequency", freq_buf, COLOR_YELLOW);

    char data_buf[32];
    if(app->last_decoded.bits <= 24) {
        lv_snprintf(data_buf, sizeof(data_buf), "0x%06lX",
                    (unsigned long)(app->last_decoded.data & 0xFFFFFF));
    } else {
        lv_snprintf(data_buf, sizeof(data_buf), "0x%08lX",
                    (unsigned long)(app->last_decoded.data & 0xFFFFFFFF));
    }
    view_info_add_field(app->info, "Data", data_buf, COLOR_PRIMARY);

    char bits_buf[16];
    lv_snprintf(bits_buf, sizeof(bits_buf), "%u bit", (unsigned)app->last_decoded.bits);
    view_info_add_field(app->info, "Size", bits_buf, COLOR_SECONDARY);

    char te_buf[16];
    lv_snprintf(te_buf, sizeof(te_buf), "%lu us", (unsigned long)app->last_decoded.te);
    view_info_add_field(app->info, "TE", te_buf, COLOR_SECONDARY);

    view_info_add_separator(app->info);

    view_info_add_button_row(app->info,
        LV_SYMBOL_UPLOAD " Send", COLOR_GREEN, send_cb, app,
        LV_SYMBOL_SAVE " Save",  COLOR_BLUE,  save_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewInfo);
}

bool subghz_scene_receiver_info_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if(event.type == SceneEventTypeCustom) {
        if(event.event == ReceiverInfoEvtSend) {
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Replay);
            return true;
        }
        if(event.event == ReceiverInfoEvtSave) {
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Save);
            return true;
        }
    }
    return false;
}

void subghz_scene_receiver_info_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_info_reset(app->info);
}
