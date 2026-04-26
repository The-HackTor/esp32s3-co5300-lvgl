#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>

static const uint32_t DELAY_PRESETS_MS[] = {
    0, 250, 500, 750, 1000, 1500, 2000, 3000, 5000,
};
#define DELAY_PRESETS_N (sizeof(DELAY_PRESETS_MS) / sizeof(DELAY_PRESETS_MS[0]))

static void delay_picked(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= DELAY_PRESETS_N) return;

    app->pending_step.delay_ms = DELAY_PRESETS_MS[index];
    if(macro_append_step(&app->current_macro, &app->pending_step) == ESP_OK) {
        macro_save(&app->current_macro);
    }
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                     ir_SCENE_MacroEdit);
}

void ir_scene_macro_pick_delay_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Delay After", COLOR_CYAN);

    char label[32];
    for(uint32_t i = 0; i < DELAY_PRESETS_N; i++) {
        if(DELAY_PRESETS_MS[i] == 0) {
            snprintf(label, sizeof(label), "None");
        } else {
            snprintf(label, sizeof(label), "%lu ms",
                     (unsigned long)DELAY_PRESETS_MS[i]);
        }
        view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, label,
                              COLOR_CYAN, i, delay_picked, app);
    }

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_pick_delay_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_pick_delay_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
