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

#define HISTORY_VIEW_MAX 16
#define CLEAR_INDEX      0xFFFFFFFEu

static IrHistoryEntry s_entries[HISTORY_VIEW_MAX];
static size_t         s_count;
static int            s_selected;

static void render_list(IrApp *app);

static void send_entry(const IrHistoryEntry *e, IrApp *app)
{
    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s", e->protocol);
    msg.address = e->address;
    msg.command = e->command;

    uint16_t *enc_t = NULL;
    size_t    enc_n = 0;
    uint32_t  enc_hz = 38000;
    if(ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz) == ESP_OK) {
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(enc_t, enc_n, enc_hz);
        hw_rgb_off();
        free(enc_t);
        return;
    }

    view_popup_reset(app->popup);
    view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
    view_popup_set_header(app->popup, "Send Pending", COLOR_YELLOW);
    view_popup_set_text(app->popup, "Encoder unavailable.");
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                            (uint32_t)TransitionFadeIn, 120);
}

static esp_err_t save_entry_as_remote(const IrHistoryEntry *e)
{
    IrRemote r = {0};
    char name[IR_REMOTE_NAME_MAX];
    snprintf(name, sizeof(name), "captured_%lld",
             (long long)(e->timestamp_us / 1000000));
    if(ir_remote_init(&r, name) != ESP_OK) return ESP_FAIL;

    IrButton btn = {0};
    snprintf(btn.name, sizeof(btn.name), "Captured");
    btn.signal.type = INFRARED_SIGNAL_PARSED;
    snprintf(btn.signal.parsed.protocol,
             sizeof(btn.signal.parsed.protocol), "%s", e->protocol);
    btn.signal.parsed.address = e->address;
    btn.signal.parsed.command = e->command;

    esp_err_t err = ir_remote_append_button(&r, &btn);
    if(err == ESP_OK) err = ir_remote_save(&r);
    ir_remote_free(&r);
    return err;
}

static void dialog_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(s_selected < 0 || (size_t)s_selected >= s_count) {
        render_list(app);
        return;
    }
    const IrHistoryEntry *e = &s_entries[s_selected];

    if(result == VIEW_DIALOG_RESULT_LEFT) {
        send_entry(e, app);
        render_list(app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                                (uint32_t)TransitionSlideRight, 120);
    } else if(result == VIEW_DIALOG_RESULT_CENTER) {
        esp_err_t err = save_entry_as_remote(e);
        view_popup_reset(app->popup);
        if(err == ESP_OK) {
            view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
            view_popup_set_header(app->popup, "Saved", COLOR_GREEN);
            view_popup_set_text(app->popup, "Captured remote written.");
        } else {
            view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
            view_popup_set_header(app->popup, "Save Failed", COLOR_RED);
            view_popup_set_text(app->popup, "Check SD card.");
        }
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
    } else {
        ir_history_delete((size_t)s_selected);
        render_list(app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                                (uint32_t)TransitionSlideRight, 120);
    }
    s_selected = -1;
}

static void row_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index == CLEAR_INDEX) {
        ir_history_clear();
        render_list(app);
        return;
    }
    if(index >= s_count) return;
    s_selected = (int)index;
    const IrHistoryEntry *e = &s_entries[index];

    char header[48];
    char body[96];
    snprintf(header, sizeof(header), "%s 0x%lX",
             e->protocol, (unsigned long)e->address);
    snprintf(body, sizeof(body), "cmd 0x%lX  T+%llds",
             (unsigned long)e->command,
             (long long)(e->timestamp_us / 1000000));

    view_dialog_reset(app->dialog);
    view_dialog_set_icon(app->dialog, LV_SYMBOL_LIST,
                         ir_protocol_color(e->protocol));
    view_dialog_set_header(app->dialog, header, COLOR_CYAN);
    view_dialog_set_text(app->dialog, body);
    view_dialog_set_left_button(app->dialog,   "Send",   COLOR_RED);
    view_dialog_set_center_button(app->dialog, "Save",   COLOR_GREEN);
    view_dialog_set_right_button(app->dialog,  "Delete", COLOR_YELLOW);
    view_dialog_set_callback(app->dialog, dialog_cb, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewDialog,
                                            (uint32_t)TransitionSlideLeft, 180);
}

static void render_list(IrApp *app)
{
    s_count = 0;
    ir_history_read(s_entries, HISTORY_VIEW_MAX, &s_count);

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "History", COLOR_GREEN);

    if(s_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Captures",
                              COLOR_DIM, 0, NULL, NULL);
        return;
    }

    char label[48];
    for(size_t i = 0; i < s_count; i++) {
        const IrHistoryEntry *e = &s_entries[i];
        snprintf(label, sizeof(label), "%s 0x%lX",
                 e->protocol, (unsigned long)e->address);
        view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, label,
                              ir_protocol_color(e->protocol),
                              (uint32_t)i, row_tapped, app);
    }
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Clear All",
                          COLOR_RED, CLEAR_INDEX, row_tapped, app);
}

void ir_scene_history_on_enter(void *ctx)
{
    IrApp *app = ctx;
    s_selected = -1;
    render_list(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_history_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_history_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_dialog_reset(app->dialog);
    view_popup_reset(app->popup);
    s_selected = -1;
}
