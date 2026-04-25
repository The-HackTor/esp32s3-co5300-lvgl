#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "store/ir_store.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/ir_codecs.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void send_universal(IrApp *app, size_t button_idx, size_t signal_idx)
{
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    const IrButton *btn = ir_universal_db_button_signal(cat, button_idx, signal_idx);
    if(!btn) return;

    if(btn->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &btn->signal.raw;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(r->timings, r->n_timings,
                       r->freq_hz ? r->freq_hz : 38000);
        hw_rgb_off();
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
    if(ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz) == ESP_OK) {
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(enc_t, enc_n, enc_hz);
        hw_rgb_off();
        free(enc_t);
    }
}

static void button_selected(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    size_t total = ir_universal_db_signal_count(cat, index);
    if(total == 0) return;

    size_t cursor_slot = index < (sizeof(app->universal_signal_cursor) /
                                  sizeof(app->universal_signal_cursor[0]))
                         ? index : 0;
    size_t cursor = app->universal_signal_cursor[cursor_slot];
    send_universal(app, index, cursor);
    app->universal_signal_cursor[cursor_slot] = (uint16_t)((cursor + 1) % total);

    char text[48];
    snprintf(text, sizeof(text), "%s  %u / %u",
             ir_universal_db_button_name(cat, index),
             (unsigned)(cursor + 1), (unsigned)total);

    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_PLAY, COLOR_RED);
    view_popup_set_header(app->popup, "Sent", COLOR_GREEN);
    view_popup_set_text(app->popup, text);
    view_popup_set_timeout(app->popup, 900, NULL, NULL);
    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
}

void ir_scene_universal_category_on_enter(void *ctx)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    size_t total_buttons = ir_universal_db_button_count(cat);

    char header[40];
    snprintf(header, sizeof(header), "%s", ir_universal_category_label(cat));

    if(total_buttons == 0) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
        view_popup_set_header(app->popup, header, COLOR_SECONDARY);
        view_popup_set_text(app->popup, "Universal DB empty.");
        view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
        return;
    }

    memset(app->universal_signal_cursor, 0, sizeof(app->universal_signal_cursor));

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, header, COLOR_ORANGE);
    for(size_t i = 0; i < total_buttons; i++) {
        const char *name = ir_universal_db_button_name(cat, i);
        size_t n = ir_universal_db_signal_count(cat, i);
        char label[40];
        snprintf(label, sizeof(label), "%s (%u)", name, (unsigned)n);
        view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, label,
                              COLOR_RED, (uint32_t)i, button_selected, app);
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_UniversalCategory));

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewSubmenu);
}

bool ir_scene_universal_category_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_universal_category_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
