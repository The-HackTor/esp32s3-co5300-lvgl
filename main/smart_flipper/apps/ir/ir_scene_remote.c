#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"

#include <string.h>

#define ADD_BUTTON_INDEX 0xFFFFFFFEu

static void button_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;

    if(index == ADD_BUTTON_INDEX) {
        if(app->pending_valid) {
            ir_button_free(&app->pending_button);
            app->pending_valid = false;
        }
        app->is_learning_new_remote = false;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Learn);
        return;
    }

    if(index >= app->current_remote.button_count) return;
    const IrButton *btn = &app->current_remote.buttons[index];

    if(btn->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &btn->signal.raw;
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

void ir_scene_remote_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, app->current_remote.name, COLOR_ORANGE);

    for(size_t i = 0; i < app->current_remote.button_count; i++) {
        const IrButton *b = &app->current_remote.buttons[i];
        view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, b->name,
                              COLOR_RED, (uint32_t)i, button_tapped, app);
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_PLUS, "Learn Another",
                          COLOR_BLUE, ADD_BUTTON_INDEX, button_tapped, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewSubmenu);
}

bool ir_scene_remote_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_remote_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
