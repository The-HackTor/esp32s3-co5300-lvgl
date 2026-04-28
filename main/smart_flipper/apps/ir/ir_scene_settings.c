#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_settings.h"
#include "hw/hw_ir.h"
#include "lib/infrared/ir_codecs.h"

#include <stdio.h>
#include <stdlib.h>

#define IDX_BRUTE_GAP    0
#define IDX_BRUTE_AC_GAP 1
#define IDX_BRUTE_REPEAT 2
#define IDX_TX_ECHO      3
#define IDX_HISTORY_MAX  4
#define IDX_AUTO_SAVE    5
#define IDX_TX_INVERT    6
#define IDX_TX_TEST      7

static void render(IrApp *app);

static uint16_t cycle_brute_gap(uint16_t cur)
{
    static const uint16_t opts[] = { 100, 150, 250 };
    for(size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
        if(cur == opts[i]) return opts[(i + 1) % (sizeof(opts)/sizeof(opts[0]))];
    }
    return opts[0];
}

static uint16_t cycle_brute_ac_gap(uint16_t cur)
{
    static const uint16_t opts[] = { 200, 250, 400, 600 };
    for(size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
        if(cur == opts[i]) return opts[(i + 1) % (sizeof(opts)/sizeof(opts[0]))];
    }
    return opts[0];
}

static uint8_t cycle_brute_repeat(uint8_t cur)
{
    static const uint8_t opts[] = { 1, 2, 3, 5 };
    for(size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
        if(cur == opts[i]) return opts[(i + 1) % (sizeof(opts)/sizeof(opts[0]))];
    }
    return 3;
}

static uint16_t cycle_echo(uint16_t cur)
{
    static const uint16_t opts[] = { 50, 100, 200, 400 };
    for(size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
        if(cur == opts[i]) return opts[(i + 1) % (sizeof(opts)/sizeof(opts[0]))];
    }
    return opts[0];
}

static uint16_t cycle_history(uint16_t cur)
{
    static const uint16_t opts[] = { 32, 64, 128, 256 };
    for(size_t i = 0; i < sizeof(opts)/sizeof(opts[0]); i++) {
        if(cur == opts[i]) return opts[(i + 1) % (sizeof(opts)/sizeof(opts[0]))];
    }
    return opts[0];
}

static void item_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    const IrSettings *cur = ir_settings();
    switch(index) {
    case IDX_BRUTE_GAP:    ir_settings_set_brute_gap_ms   (cycle_brute_gap   (cur->brute_gap_ms));    break;
    case IDX_BRUTE_AC_GAP: ir_settings_set_brute_ac_gap_ms(cycle_brute_ac_gap(cur->brute_ac_gap_ms)); break;
    case IDX_BRUTE_REPEAT: ir_settings_set_brute_repeat   (cycle_brute_repeat(cur->brute_repeat));    break;
    case IDX_TX_ECHO:      ir_settings_set_tx_echo_ms     (cycle_echo        (cur->tx_echo_ms));      break;
    case IDX_HISTORY_MAX:  ir_settings_set_history_max    (cycle_history     (cur->history_max));    break;
    case IDX_AUTO_SAVE:    ir_settings_set_auto_save      (!cur->auto_save_worked);                  break;
    case IDX_TX_INVERT:
        ir_settings_set_tx_invert(!cur->tx_invert);
        hw_ir_set_invert(ir_settings()->tx_invert);
        break;
    case IDX_TX_TEST: {
        /* Bench diagnostic: fire NEC(addr=0x04, cmd=0x08) ten times with
         * 110ms gaps. Phone camera should show IR pulsing; Flipper Zero in
         * Learn mode should decode it as "NEC 04 08". Isolates "no IR
         * coming out" vs "IR present but TV ignores it".
         *
         * RX must be paused for the duration -- self-echo decode races the
         * latent RC6 free-path heap corruption. The TX self-test here fires
         * for ~2s; ir_app_rx_resume runs from a one-shot lv_timer 2500ms
         * later. ir_app_rx_pause/_resume are refcounted so nesting is safe. */
        ir_app_rx_pause();

        IrDecoded msg = { .source = IR_DECODED_FLIPPER, .address = 0x04, .command = 0x08 };
        snprintf(msg.protocol, sizeof(msg.protocol), "NEC");
        uint16_t *t = NULL;
        size_t    n = 0;
        uint32_t  hz = 38000;
        if(ir_codecs_encode(&msg, &t, &n, &hz) == ESP_OK && t && n) {
            HwIrTxRequest req = {
                .timings    = t,
                .n_timings  = n,
                .carrier_hz = hz,
                .repeat     = 10,
                .gap_ms     = 110,
                .on_done    = NULL,
                .ctx        = NULL,
            };
            hw_ir_tx_submit(&req);
        }
        if(t) free(t);

        lv_timer_t *resume_timer = lv_timer_create(
            (void(*)(lv_timer_t *))ir_app_rx_resume_then_delete_timer, 2500, NULL);
        (void)resume_timer;

        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
        view_popup_set_header(app->popup, "TX Test Firing", COLOR_GREEN);
        view_popup_set_text(app->popup, "NEC 04/08 x10");
        view_popup_set_timeout(app->popup, 1500, NULL, NULL);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher,
                                                IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }
    default:               return;
    }
    render(app);
}

static void render(IrApp *app)
{
    const IrSettings *s = ir_settings();
    char buf[40];

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Settings", COLOR_CYAN);

    snprintf(buf, sizeof(buf), "Brute gap:  %u ms", (unsigned)s->brute_gap_ms);
    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, buf, COLOR_CYAN,
                          IDX_BRUTE_GAP, item_tapped, app);

    snprintf(buf, sizeof(buf), "AC brute gap:  %u ms", (unsigned)s->brute_ac_gap_ms);
    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, buf, COLOR_CYAN,
                          IDX_BRUTE_AC_GAP, item_tapped, app);

    snprintf(buf, sizeof(buf), "Brute repeat:  %ux", (unsigned)s->brute_repeat);
    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, buf, COLOR_ORANGE,
                          IDX_BRUTE_REPEAT, item_tapped, app);

    snprintf(buf, sizeof(buf), "TX echo gate:  %u ms", (unsigned)s->tx_echo_ms);
    view_submenu_add_item(app->submenu, LV_SYMBOL_BELL, buf, COLOR_ORANGE,
                          IDX_TX_ECHO, item_tapped, app);

    snprintf(buf, sizeof(buf), "History cap:  %u", (unsigned)s->history_max);
    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, buf, COLOR_GREEN,
                          IDX_HISTORY_MAX, item_tapped, app);

    snprintf(buf, sizeof(buf), "Auto-save Worked!:  %s",
             s->auto_save_worked ? "Yes" : "No");
    view_submenu_add_item(app->submenu, LV_SYMBOL_SAVE, buf,
                          s->auto_save_worked ? COLOR_GREEN : COLOR_DIM,
                          IDX_AUTO_SAVE, item_tapped, app);

    snprintf(buf, sizeof(buf), "TX invert:  %s",
             s->tx_invert ? "On" : "Off");
    view_submenu_add_item(app->submenu, LV_SYMBOL_LOOP, buf,
                          s->tx_invert ? COLOR_ORANGE : COLOR_DIM,
                          IDX_TX_INVERT, item_tapped, app);

    view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, "TX Self-Test (NEC 04/08)",
                          COLOR_CYAN, IDX_TX_TEST, item_tapped, app);
}

void ir_scene_settings_on_enter(void *ctx)
{
    IrApp *app = ctx;
    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_settings_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_settings_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
