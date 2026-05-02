#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/ir_protocol_color.h"

#include "esp_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_VIEW_MAX  64
#define CLEAR_INDEX       0xFFFFFFFEu
#define FILTER_INDEX      0xFFFFFFFDu

#define FILTER_ALL    0
#define FILTER_HOUR   1
#define FILTER_RECENT 2

typedef struct {
    char     protocol[IR_PROTOCOL_NAME_MAX];
    uint32_t address;
    uint32_t command;
    int64_t  first_us;
    int64_t  last_us;
    uint32_t count;
    int      first_entry_idx;
} HistoryGroup;

static IrHistoryEntry s_entries[HISTORY_VIEW_MAX];
static size_t         s_count;
static HistoryGroup   s_groups[HISTORY_VIEW_MAX];
static size_t         s_group_count;
static int            s_selected_entry;
static int            s_filter;

static void render_list(IrApp *app);

static void format_relative(int64_t age_us, char *buf, size_t cap)
{
    int64_t s = age_us / 1000000;
    if(s < 60)        snprintf(buf, cap, "%llds ago",  (long long)s);
    else if(s < 3600) snprintf(buf, cap, "%lldm ago",  (long long)(s / 60));
    else              snprintf(buf, cap, "%lldh ago",  (long long)(s / 3600));
}

static const char *filter_label(int f)
{
    switch(f) {
    case FILTER_HOUR:   return "Last hour";
    case FILTER_RECENT: return "Last 5 min";
    default:            return "All";
    }
}

static int64_t filter_threshold_us(int f)
{
    switch(f) {
    case FILTER_HOUR:   return 3600LL * 1000000LL;
    case FILTER_RECENT: return  300LL * 1000000LL;
    default:            return INT64_MAX;
    }
}

static void rebuild_groups(void)
{
    s_group_count = 0;
    int64_t now = esp_timer_get_time();
    int64_t threshold = filter_threshold_us(s_filter);

    for(size_t i = 0; i < s_count; i++) {
        const IrHistoryEntry *e = &s_entries[i];
        if((now - e->timestamp_us) > threshold) continue;

        bool merged = false;
        for(size_t g = 0; g < s_group_count; g++) {
            HistoryGroup *gp = &s_groups[g];
            if(strcmp(gp->protocol, e->protocol) == 0 &&
               gp->address == e->address && gp->command == e->command) {
                gp->count++;
                if(e->timestamp_us > gp->last_us)  gp->last_us  = e->timestamp_us;
                if(e->timestamp_us < gp->first_us) gp->first_us = e->timestamp_us;
                merged = true;
                break;
            }
        }
        if(!merged && s_group_count < HISTORY_VIEW_MAX) {
            HistoryGroup *gp = &s_groups[s_group_count++];
            snprintf(gp->protocol, sizeof(gp->protocol), "%s", e->protocol);
            gp->address  = e->address;
            gp->command  = e->command;
            gp->first_us = e->timestamp_us;
            gp->last_us  = e->timestamp_us;
            gp->count    = 1;
            gp->first_entry_idx = (int)i;
        }
    }

    for(size_t i = 1; i < s_group_count; i++) {
        for(size_t j = i; j > 0 && s_groups[j-1].last_us < s_groups[j].last_us; j--) {
            HistoryGroup tmp = s_groups[j-1];
            s_groups[j-1]    = s_groups[j];
            s_groups[j]      = tmp;
        }
    }
}

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
        ir_recents_append("History", e->protocol, e->address, e->command);
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
    if(s_selected_entry < 0 || (size_t)s_selected_entry >= s_count) {
        render_list(app);
        return;
    }
    const IrHistoryEntry *e = &s_entries[s_selected_entry];

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
        ir_history_delete((size_t)s_selected_entry);
        s_count = 0;
        ir_history_read(s_entries, HISTORY_VIEW_MAX, &s_count);
        rebuild_groups();
        render_list(app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                                (uint32_t)TransitionSlideRight, 120);
    }
    s_selected_entry = -1;
}

static void row_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;

    if(index == FILTER_INDEX) {
        s_filter = (s_filter + 1) % 3;
        rebuild_groups();
        render_list(app);
        return;
    }
    if(index == CLEAR_INDEX) {
        ir_history_clear();
        s_count = 0;
        s_group_count = 0;
        render_list(app);
        return;
    }
    if(index >= s_group_count) return;

    const HistoryGroup *gp = &s_groups[index];
    s_selected_entry = gp->first_entry_idx;
    const IrHistoryEntry *e = &s_entries[s_selected_entry];

    char header[48];
    char age[24];
    char body[96];
    snprintf(header, sizeof(header), "%s 0x%lX",
             e->protocol, (unsigned long)e->address);
    format_relative(esp_timer_get_time() - gp->last_us, age, sizeof(age));
    snprintf(body, sizeof(body), "cmd 0x%lX\nx%u  -  %s",
             (unsigned long)e->command, (unsigned)gp->count, age);

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
    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "History", COLOR_GREEN);

    char filter_label_buf[32];
    snprintf(filter_label_buf, sizeof(filter_label_buf), "Filter:  %s",
             filter_label(s_filter));
    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, filter_label_buf,
                          COLOR_BLUE, FILTER_INDEX, row_tapped, app);

    if(s_group_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Captures",
                              COLOR_DIM, 0, NULL, NULL);
        return;
    }

    int64_t now = esp_timer_get_time();
    char label[48];
    char sub[40];
    for(size_t i = 0; i < s_group_count; i++) {
        const HistoryGroup *gp = &s_groups[i];
        snprintf(label, sizeof(label), "%s 0x%lX",
                 gp->protocol, (unsigned long)gp->address);
        char age[20];
        format_relative(now - gp->last_us, age, sizeof(age));
        snprintf(sub, sizeof(sub), "x%u  -  %s", (unsigned)gp->count, age);
        view_submenu_add_item_ex(app->submenu, LV_SYMBOL_PLAY, label, sub,
                                 ir_protocol_color(gp->protocol),
                                 (uint32_t)i, row_tapped, NULL, app);
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Clear All",
                          COLOR_RED, CLEAR_INDEX, row_tapped, app);
}

void ir_scene_history_on_enter(void *ctx)
{
    IrApp *app = ctx;
    s_selected_entry = -1;

    s_count = 0;
    ir_history_read(s_entries, HISTORY_VIEW_MAX, &s_count);
    rebuild_groups();
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
    s_selected_entry = -1;
}
