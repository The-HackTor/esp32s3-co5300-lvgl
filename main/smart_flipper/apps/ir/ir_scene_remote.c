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
#include <string.h>

#define ADD_BUTTON_INDEX  0xFFFFFFFEu
#define EDIT_BUTTON_INDEX 0xFFFFFFFDu

#define IR_REPEAT_PERIOD_MS 110

static void pseudo_tapped(void *ctx, uint32_t index)
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
    if(index == EDIT_BUTTON_INDEX) {
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Edit);
    }
}

static void button_pressed(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= app->current_remote.button_count) return;
    const IrButton *btn = &app->current_remote.buttons[index];

    hw_rgb_set(255, 0, 0);

    if(btn->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &btn->signal.raw;
        hw_ir_send_repeat_start(r->timings, r->n_timings,
                                r->freq_hz ? r->freq_hz : 38000,
                                IR_REPEAT_PERIOD_MS);
        ir_recents_append(btn->name, "RAW", 0, 0);
        return;
    }

    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s",
             btn->signal.parsed.protocol);
    msg.address = btn->signal.parsed.address;
    msg.command = btn->signal.parsed.command;

    uint16_t *enc_t = NULL;
    size_t    enc_n = 0;
    uint32_t  enc_hz = 38000;
    esp_err_t err = ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz);
    if(err == ESP_OK) {
        hw_ir_send_repeat_start(enc_t, enc_n, enc_hz, IR_REPEAT_PERIOD_MS);
        free(enc_t);
        ir_recents_append(btn->name, msg.protocol, msg.address, msg.command);
        return;
    }

    hw_rgb_off();
    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_popup_set_header(app->popup, "Encoder Pending", COLOR_YELLOW);
    view_popup_set_text(app->popup,
        "codec_db protocols need a per-protocol sender.");
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
}

static void button_released(void *ctx, uint32_t index)
{
    (void)ctx;
    (void)index;
    hw_ir_send_repeat_stop();
    hw_rgb_off();
}

void ir_scene_remote_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, app->current_remote.name, COLOR_ORANGE);

    for(size_t i = 0; i < app->current_remote.button_count; i++) {
        const IrButton *b = &app->current_remote.buttons[i];
        lv_color_t color = (b->signal.type == INFRARED_SIGNAL_PARSED)
            ? ir_protocol_color(b->signal.parsed.protocol)
            : COLOR_YELLOW;
        view_submenu_add_item_holdable(app->submenu, LV_SYMBOL_PLAY, b->name,
                                       color, (uint32_t)i,
                                       button_pressed, button_released, app);
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_PLUS, "Learn Another",
                          COLOR_BLUE, ADD_BUTTON_INDEX, pseudo_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SETTINGS, "Edit",
                          COLOR_YELLOW, EDIT_BUTTON_INDEX, pseudo_tapped, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
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
    hw_ir_send_repeat_stop();
    hw_rgb_off();
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
