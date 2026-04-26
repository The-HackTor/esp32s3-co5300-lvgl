#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "lib/infrared/ir_protocol_color.h"

#include <stdio.h>
#include <string.h>

static IrRemote s_remote;
static bool     s_remote_loaded;

static void button_picked(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(!s_remote_loaded || index >= s_remote.button_count) return;
    snprintf(app->pending_step.button, sizeof(app->pending_step.button),
             "%s", s_remote.buttons[index].name);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroPickDelay);
}

void ir_scene_macro_pick_button_on_enter(void *ctx)
{
    IrApp *app = ctx;

    if(s_remote_loaded) {
        ir_remote_free(&s_remote);
        s_remote_loaded = false;
    }

    char path[IR_REMOTE_PATH_MAX];
    esp_err_t err = ir_store_remote_path(path, sizeof(path),
                                         app->pending_step.remote);
    if(err == ESP_OK) {
        err = ir_remote_load(&s_remote, path);
        if(err == ESP_OK) s_remote_loaded = true;
    }

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Pick Button", COLOR_CYAN);

    if(!s_remote_loaded || s_remote.button_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Buttons",
                              COLOR_DIM, 0xFFFFFFFFu, NULL, NULL);
    } else {
        for(size_t i = 0; i < s_remote.button_count; i++) {
            const IrButton *b = &s_remote.buttons[i];
            lv_color_t color = (b->signal.type == INFRARED_SIGNAL_PARSED)
                ? ir_protocol_color(b->signal.parsed.protocol)
                : COLOR_YELLOW;
            view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, b->name,
                                  color, (uint32_t)i, button_picked, app);
        }
    }

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_pick_button_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_pick_button_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    if(s_remote_loaded) {
        ir_remote_free(&s_remote);
        s_remote_loaded = false;
    }
}
