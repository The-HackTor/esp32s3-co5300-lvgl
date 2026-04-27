#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_settings.h"

#include <stdio.h>

#define IDX_BRUTE_GAP    0
#define IDX_BRUTE_AC_GAP 1
#define IDX_TX_ECHO      2
#define IDX_HISTORY_MAX  3
#define IDX_AUTO_SAVE    4

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
    case IDX_TX_ECHO:      ir_settings_set_tx_echo_ms     (cycle_echo        (cur->tx_echo_ms));      break;
    case IDX_HISTORY_MAX:  ir_settings_set_history_max    (cycle_history     (cur->history_max));    break;
    case IDX_AUTO_SAVE:    ir_settings_set_auto_save      (!cur->auto_save_worked);                  break;
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
