#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>

static const uint8_t REPEAT_PRESETS[] = { 1, 2, 3, 5, 10 };
#define REPEAT_PRESETS_N (sizeof(REPEAT_PRESETS) / sizeof(REPEAT_PRESETS[0]))

static void repeat_picked(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= REPEAT_PRESETS_N) return;

    app->pending_step.repeat = REPEAT_PRESETS[index];
    if(macro_append_step(&app->current_macro, &app->pending_step) == ESP_OK) {
        macro_save(&app->current_macro);
    }
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                     ir_SCENE_MacroEdit);
}

void ir_scene_macro_pick_repeat_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Repeat", COLOR_YELLOW);

    char label[24];
    for(uint32_t i = 0; i < REPEAT_PRESETS_N; i++) {
        if(REPEAT_PRESETS[i] == 1) {
            snprintf(label, sizeof(label), "Once");
        } else {
            snprintf(label, sizeof(label), "%u times", (unsigned)REPEAT_PRESETS[i]);
        }
        view_submenu_add_item(app->submenu, LV_SYMBOL_LOOP, label,
                              COLOR_YELLOW, i, repeat_picked, app);
    }

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_pick_repeat_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_pick_repeat_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
