#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"

#include <stdio.h>

static void btn_save(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_LearnEnterName);
}

static void btn_send(void *ctx)
{
    IrApp *app = ctx;
    if(!app->pending_valid) return;

    if(app->pending_button.signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &app->pending_button.signal.raw;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(r->timings, r->n_timings,
                       r->freq_hz ? r->freq_hz : 38000);
        hw_rgb_off();
    } else {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
        view_popup_set_header(app->popup, "Encoder Pending", COLOR_YELLOW);
        view_popup_set_text(app->popup,
            "Replay of decoded protocols ships in the next slice.");
        view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
    }
}

static void btn_discard(void *ctx)
{
    IrApp *app = ctx;
    if(app->pending_valid) {
        ir_button_free(&app->pending_button);
        app->pending_valid = false;
    }
    scene_manager_previous_scene(&app->scene_mgr);
}

void ir_scene_learn_success_on_enter(void *ctx)
{
    IrApp *app = ctx;
    char addr_buf[24];
    char cmd_buf[24];

    view_info_reset(app->info);

    if(app->last_decoded_valid) {
        view_info_set_header(app->info, app->last_decoded.protocol, COLOR_GREEN);
        snprintf(addr_buf, sizeof(addr_buf), "0x%08lX",
                 (unsigned long)app->last_decoded.address);
        snprintf(cmd_buf, sizeof(cmd_buf), "0x%08lX",
                 (unsigned long)app->last_decoded.command);
        view_info_add_field(app->info, "Address", addr_buf, COLOR_PRIMARY);
        view_info_add_field(app->info, "Command", cmd_buf, COLOR_PRIMARY);
    } else {
        view_info_set_header(app->info, "Raw Capture", COLOR_YELLOW);
        char cnt_buf[32];
        snprintf(cnt_buf, sizeof(cnt_buf), "%u",
                 (unsigned)app->pending_button.signal.raw.n_timings);
        view_info_add_field(app->info, "Timings", cnt_buf, COLOR_PRIMARY);
        view_info_add_field(app->info, "Carrier", "38 kHz", COLOR_SECONDARY);
    }

    view_info_add_button(app->info, LV_SYMBOL_SAVE " Save", COLOR_GREEN, btn_save, app);
    view_info_add_button_row(app->info,
                             LV_SYMBOL_PLAY " Send", COLOR_RED, btn_send, app,
                             LV_SYMBOL_TRASH " Discard", COLOR_DIM, btn_discard, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewInfo);
}

bool ir_scene_learn_success_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_learn_success_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_info_reset(app->info);
    view_popup_reset(app->popup);
}
