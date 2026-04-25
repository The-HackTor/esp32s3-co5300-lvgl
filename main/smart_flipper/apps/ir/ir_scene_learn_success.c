#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/ir_protocol_color.h"

#include <stdio.h>
#include <stdlib.h>

static void btn_save(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_LearnEnterName);
}

static void btn_send(void *ctx)
{
    IrApp *app = ctx;
    if(!app->pending_valid) return;

    if(app->pending_raw_timings && app->pending_raw_n > 0) {
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(app->pending_raw_timings, app->pending_raw_n, 38000);
        hw_rgb_off();
        return;
    }
    if(app->pending_button.signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &app->pending_button.signal.raw;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(r->timings, r->n_timings,
                       r->freq_hz ? r->freq_hz : 38000);
        hw_rgb_off();
        return;
    }

    uint16_t *enc_t = NULL;
    size_t    enc_n = 0;
    uint32_t  enc_hz = 38000;
    esp_err_t err = ir_codecs_encode(&app->last_decoded, &enc_t, &enc_n, &enc_hz);
    if(err == ESP_OK) {
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(enc_t, enc_n, enc_hz);
        hw_rgb_off();
        free(enc_t);
        return;
    }

    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_popup_set_header(app->popup, "Encoder Pending", COLOR_YELLOW);
    view_popup_set_text(app->popup,
        "codec_db protocols need a per-protocol sender.");
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
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

    lv_color_t accent = COLOR_YELLOW;
    if(app->last_decoded_valid) {
        accent = ir_protocol_color(app->last_decoded.protocol);
        view_info_set_header(app->info, app->last_decoded.protocol, accent);
        view_info_add_pill(app->info, app->last_decoded.protocol, accent);
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

    if(app->pending_raw_timings && app->pending_raw_n > 0) {
        view_info_add_waveform(app->info, app->pending_raw_timings,
                               app->pending_raw_n, accent);
    }

    view_info_add_button(app->info, LV_SYMBOL_SAVE " Save", COLOR_GREEN, btn_save, app);
    view_info_add_button_row(app->info,
                             LV_SYMBOL_PLAY " Send", COLOR_RED, btn_send, app,
                             LV_SYMBOL_TRASH " Discard", COLOR_DIM, btn_discard, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewInfo,
                                            (uint32_t)TransitionSlideLeft, 180);
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
