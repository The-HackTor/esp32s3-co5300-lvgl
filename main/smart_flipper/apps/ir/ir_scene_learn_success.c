#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/ir_protocol_color.h"
#include "lib/infrared/universal_db/ir_universal_index.h"
#include "lib/infrared/encoder_decoder/infrared.h"

#include <stdio.h>
#include <stdlib.h>

/* Format an N-bit value: 0x1F (5b) for short widths, 0xABCD (16b) for
 * wider, full 0x12345678 (32b) for the catch-all. Bits include the actual
 * mask width per Flipper's spec, so users see the meaningful digits not
 * the leading zeroes from a 32-bit container. */
static void format_bits(char *out, size_t cap, uint32_t value, uint8_t bits)
{
    if(bits == 0 || bits > 32) bits = 32;
    int hex_chars = (bits + 3) / 4;
    uint32_t mask = (bits == 32) ? 0xFFFFFFFFU : ((1U << bits) - 1U);
    snprintf(out, cap, "0x%0*lX  (%u-bit)", hex_chars,
             (unsigned long)(value & mask), (unsigned)bits);
}

static uint32_t sum_durations_us(const uint16_t *t, size_t n)
{
    uint32_t s = 0;
    for(size_t i = 0; i < n; i++) s += t[i];
    return s;
}

static IrUniversalCategory s_match_cat;
static bool                s_match_valid;
static char                s_match_label[IR_REMOTE_NAME_MAX];
static IrMatchConfidence   s_match_conf;
static uint16_t            s_match_group_size;

static void btn_save(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_LearnEnterName);
}

static void btn_send(void *ctx)
{
    IrApp *app = ctx;
    if(!app->pending_valid) return;

    /* Prefer the canonical encoded form when we have a parsed decode.
     * pending_raw_timings is the verbatim TSOP capture: the first sample's
     * mark/space identity depends on when the RMT RX channel armed relative
     * to the incoming frame, and the carrier was hardcoded to 38 kHz
     * regardless of what was actually captured. Encoding from the decoded
     * (protocol, address, command) tuple produces a clean frame identical
     * to what a real remote emits. */
    if(app->last_decoded_valid &&
       app->pending_button.signal.type == INFRARED_SIGNAL_PARSED) {
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
        /* Encode failed (codec_db best-fit without a sender, etc.) -- fall
         * through to raw replay so the user can still try the captured
         * envelope. */
        if(enc_t) free(enc_t);
    }

    /* Raw fallback. Use the actual captured carrier when available rather
     * than always 38 kHz; the pending_button.signal.raw.freq_hz is the
     * authoritative value rx_drain_timer_cb wrote when the frame arrived. */
    if(app->pending_button.signal.type == INFRARED_SIGNAL_RAW &&
       app->pending_button.signal.raw.timings &&
       app->pending_button.signal.raw.n_timings > 0) {
        const InfraredSignalRaw *r = &app->pending_button.signal.raw;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(r->timings, r->n_timings,
                       r->freq_hz ? r->freq_hz : 38000);
        hw_rgb_off();
        return;
    }

    if(app->pending_raw_timings && app->pending_raw_n > 0) {
        uint32_t hz = app->pending_button.signal.raw.freq_hz
                          ? app->pending_button.signal.raw.freq_hz : 38000;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(app->pending_raw_timings, app->pending_raw_n, hz);
        hw_rgb_off();
        return;
    }

    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_popup_set_header(app->popup, "Nothing to send", COLOR_YELLOW);
    view_popup_set_text(app->popup,
        "Capture a signal first or pick a different protocol.");
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

static void btn_load_universal(void *ctx)
{
    IrApp *app = ctx;
    if(!s_match_valid) return;
    app->universal_category = (int)s_match_cat;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_Universal,
                                  (uint32_t)s_match_cat);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_UniversalCategory);
}

void ir_scene_learn_success_on_enter(void *ctx)
{
    IrApp *app = ctx;
    char addr_buf[24];
    char cmd_buf[24];

    view_info_reset(app->info);

    s_match_valid = false;
    s_match_label[0] = '\0';
    s_match_conf = IR_MATCH_NONE;
    s_match_group_size = 0;

    lv_color_t accent = COLOR_YELLOW;
    char buf[48];

    if(app->last_decoded_valid) {
        accent = ir_protocol_color(app->last_decoded.protocol);
        view_info_set_header(app->info, app->last_decoded.protocol, accent);
        view_info_add_pill(app->info, app->last_decoded.protocol, accent);

        const char *conf_text = (app->last_decoded.source == IR_DECODED_FLIPPER)
                                    ? "Verified" : "Best-fit";
        lv_color_t  conf_clr  = (app->last_decoded.source == IR_DECODED_FLIPPER)
                                    ? COLOR_GREEN : COLOR_YELLOW;
        view_info_add_pill(app->info, conf_text, conf_clr);

        if(app->last_decoded.repeat) {
            view_info_add_pill(app->info, "Repeat", COLOR_CYAN);
        }

        InfraredProtocol p = infrared_get_protocol_by_name(app->last_decoded.protocol);
        uint8_t addr_bits = infrared_is_protocol_valid(p)
                                ? infrared_get_protocol_address_length(p) : 32;
        uint8_t cmd_bits  = infrared_is_protocol_valid(p)
                                ? infrared_get_protocol_command_length(p) : 32;

        format_bits(addr_buf, sizeof(addr_buf), app->last_decoded.address, addr_bits);
        format_bits(cmd_buf,  sizeof(cmd_buf),  app->last_decoded.command, cmd_bits);
        view_info_add_field(app->info, "Address", addr_buf, COLOR_PRIMARY);
        view_info_add_field(app->info, "Command", cmd_buf, COLOR_PRIMARY);

        if(infrared_is_protocol_valid(p)) {
            uint32_t hz = infrared_get_protocol_frequency(p);
            snprintf(buf, sizeof(buf), "%lu.%02lu kHz",
                     (unsigned long)(hz / 1000),
                     (unsigned long)((hz % 1000) / 10));
            view_info_add_field(app->info, "Carrier", buf, COLOR_SECONDARY);
        }

        s_match_valid = ir_universal_index_match(app->last_decoded.protocol,
                                                 app->last_decoded.address,
                                                 app->last_decoded.command,
                                                 &s_match_cat,
                                                 s_match_label,
                                                 sizeof(s_match_label),
                                                 &s_match_conf,
                                                 &s_match_group_size);
    } else {
        view_info_set_header(app->info, "Raw Capture", COLOR_YELLOW);
        view_info_add_pill(app->info, "Raw only", COLOR_ORANGE);

        size_t  n_t   = app->pending_button.signal.raw.n_timings;
        uint32_t freq = app->pending_button.signal.raw.freq_hz
                            ? app->pending_button.signal.raw.freq_hz : 38000;
        uint32_t duty = app->pending_button.signal.raw.duty_pct
                            ? app->pending_button.signal.raw.duty_pct : 33;

        snprintf(buf, sizeof(buf), "%u", (unsigned)n_t);
        view_info_add_field(app->info, "Timings", buf, COLOR_PRIMARY);

        if(app->pending_raw_timings && app->pending_raw_n > 0) {
            uint32_t total_us = sum_durations_us(app->pending_raw_timings,
                                                 app->pending_raw_n);
            snprintf(buf, sizeof(buf), "%lu.%lu ms",
                     (unsigned long)(total_us / 1000),
                     (unsigned long)((total_us % 1000) / 100));
            view_info_add_field(app->info, "Duration", buf, COLOR_SECONDARY);
        }

        snprintf(buf, sizeof(buf), "%lu.%02lu kHz",
                 (unsigned long)(freq / 1000),
                 (unsigned long)((freq % 1000) / 10));
        view_info_add_field(app->info, "Carrier", buf, COLOR_SECONDARY);

        snprintf(buf, sizeof(buf), "%lu%%", (unsigned long)duty);
        view_info_add_field(app->info, "Duty", buf, COLOR_SECONDARY);
    }

    if(app->pending_raw_timings && app->pending_raw_n > 0) {
        view_info_add_waveform(app->info, app->pending_raw_timings,
                               app->pending_raw_n, accent);
    }

    if(s_match_valid) {
        char match_btn[48];
        snprintf(match_btn, sizeof(match_btn), LV_SYMBOL_LIST " Open %s",
                 ir_universal_category_label(s_match_cat));
        view_info_add_button(app->info, match_btn, COLOR_GREEN,
                             btn_load_universal, app);
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
    IrApp *app = ctx;
    if(event.type == SceneEventTypeBack &&
       (app->pending_valid || app->last_decoded_valid)) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_AskBack);
        return true;
    }
    return false;
}

void ir_scene_learn_success_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_info_reset(app->info);
    view_popup_reset(app->popup);
}
