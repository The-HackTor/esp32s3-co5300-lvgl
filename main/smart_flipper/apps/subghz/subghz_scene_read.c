#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "ui/widgets/status_bar.h"
#include <string.h>

static lv_obj_t *list_cont;
static lv_obj_t *count_lbl;
static lv_obj_t *freq_lbl;
static uint16_t displayed_count;
static lv_timer_t *hop_timer;
static uint32_t last_hop_freq;
static bool last_hop_locked;

static lv_color_t protocol_color(const char *name, uint8_t proto_type)
{
    if(proto_type == 1) return COLOR_YELLOW;

    if(strcmp(name, "Princeton") == 0)   return COLOR_CYAN;
    if(strcmp(name, "CAME") == 0)        return COLOR_ORANGE;
    if(strcmp(name, "Nice FLO") == 0)    return COLOR_GREEN;
    if(strcmp(name, "Holtek") == 0)      return COLOR_YELLOW;
    if(strcmp(name, "Hormann") == 0)     return COLOR_MAGENTA;
    if(strcmp(name, "Gate TX") == 0)     return COLOR_ORANGE;
    if(strcmp(name, "Linear") == 0)      return COLOR_CYAN;
    if(strcmp(name, "Chamberlain") == 0) return COLOR_BLUE;
    if(strcmp(name, "KeeLoq") == 0)      return COLOR_YELLOW;
    if(strcmp(name, "Somfy") == 0)       return COLOR_MAGENTA;
    if(strcmp(name, "SecPlus_V2") == 0)  return COLOR_RED;
    if(strcmp(name, "Nice_FlorS") == 0)  return COLOR_GREEN;
    if(strcmp(name, "BinRAW") == 0)      return COLOR_DIM;
    return COLOR_PRIMARY;
}

static void card_click_cb(lv_event_t *e)
{
    SubghzApp *app = lv_event_get_user_data(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));

    uint16_t count;
    const SubghzHistoryEntry *history = hw_subghz_get_read_history(&count);
    if(idx < 0 || idx >= count) return;

    const SubghzHistoryEntry *entry = &history[idx];
    app->last_decoded.protocol = entry->protocol;
    app->last_decoded.data = entry->data;
    app->last_decoded.bits = entry->bits;
    app->last_decoded.te = entry->te;
    app->decoded_valid = true;
    app->frequency = entry->freq_khz;

    scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_ReceiverInfo);
}

static void add_entry_card(lv_obj_t *parent, const SubghzHistoryEntry *entry,
                           int index, SubghzApp *app)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 2, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(card, (void *)(intptr_t)index);
    lv_obj_add_event_cb(card, card_click_cb, LV_EVENT_CLICKED, app);

    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *proto_lbl = lv_label_create(row);
    lv_label_set_text(proto_lbl, entry->protocol);
    lv_obj_set_style_text_color(proto_lbl, protocol_color(entry->protocol, entry->proto_type), 0);
    lv_obj_set_style_text_font(proto_lbl, FONT_BODY, 0);

    char right_buf[24];
    uint32_t mhz = entry->freq_khz / 1000;
    uint32_t khz = entry->freq_khz % 1000;
    if(entry->repeat_count > 1) {
        lv_snprintf(right_buf, sizeof(right_buf), "%lu.%02lu x%u",
                    (unsigned long)mhz, (unsigned long)(khz / 10),
                    entry->repeat_count);
    } else {
        lv_snprintf(right_buf, sizeof(right_buf), "%lu.%02lu",
                    (unsigned long)mhz, (unsigned long)(khz / 10));
    }
    lv_obj_t *freq_r = lv_label_create(row);
    lv_label_set_text(freq_r, right_buf);
    lv_obj_set_style_text_color(freq_r, COLOR_DIM, 0);
    lv_obj_set_style_text_font(freq_r, FONT_SMALL, 0);

    char data_buf[40];
    if(entry->bits <= 24) {
        lv_snprintf(data_buf, sizeof(data_buf), "0x%06lX  %ubit  TE:%lu",
                    (unsigned long)(entry->data & 0xFFFFFF),
                    entry->bits, (unsigned long)entry->te);
    } else {
        lv_snprintf(data_buf, sizeof(data_buf), "0x%08lX  %ubit  TE:%lu",
                    (unsigned long)(entry->data & 0xFFFFFFFF),
                    entry->bits, (unsigned long)entry->te);
    }

    lv_obj_t *data_lbl = lv_label_create(card);
    lv_label_set_text(data_lbl, data_buf);
    lv_obj_set_style_text_color(data_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(data_lbl, FONT_MONO, 0);

    if(entry->proto_type == 1 && entry->serial != 0) {
        char detail[48];
        lv_snprintf(detail, sizeof(detail), "SN:%06lX BTN:%u CNT:%u",
                    (unsigned long)entry->serial,
                    (unsigned)entry->button,
                    (unsigned)entry->counter);
        lv_obj_t *detail_lbl = lv_label_create(card);
        lv_label_set_text(detail_lbl, detail);
        lv_obj_set_style_text_color(detail_lbl, COLOR_YELLOW, 0);
        lv_obj_set_style_text_font(detail_lbl, FONT_MONO, 0);
    }
}

static void rebuild_list(SubghzApp *app)
{
    uint16_t count;
    const SubghzHistoryEntry *history = hw_subghz_get_read_history(&count);

    if(count == displayed_count && count > 0) {
        const SubghzHistoryEntry *last = &history[count - 1];
        if(last->repeat_count > 1) {
            uint32_t child_count = lv_obj_get_child_count(list_cont);
            if(child_count > 0) {
                lv_obj_t *last_card = lv_obj_get_child(list_cont,
                                                        child_count - 1);
                lv_obj_t *row = lv_obj_get_child(last_card, 0);
                if(row && lv_obj_get_child_count(row) >= 2) {
                    lv_obj_t *rep_lbl = lv_obj_get_child(row, 1);
                    char rep[24];
                    uint32_t mhz = last->freq_khz / 1000;
                    uint32_t khz = last->freq_khz % 1000;
                    lv_snprintf(rep, sizeof(rep), "%lu.%02lu x%u",
                                (unsigned long)mhz,
                                (unsigned long)(khz / 10),
                                last->repeat_count);
                    lv_label_set_text(rep_lbl, rep);
                }
            }
        }
        return;
    }

    while(displayed_count < count) {
        add_entry_card(list_cont, &history[displayed_count],
                       displayed_count, app);
        displayed_count++;
    }

    lv_obj_scroll_to_y(list_cont, LV_COORD_MAX, LV_ANIM_ON);

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%u signal%s found",
                count, count == 1 ? "" : "s");
    lv_label_set_text(count_lbl, buf);
}

static void read_cb(const SubghzHistoryEntry *entry, uint16_t total_count,
                    void *ctx)
{
    (void)entry;
    (void)total_count;
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_READ_UPDATE);
}

static void update_freq_status(SubghzApp *app)
{
    if(!freq_lbl) return;
    char buf[48];

    if(app->hopping) {
        uint32_t f = hw_subghz_hop_current_freq();
        uint32_t mhz = f / 1000;
        uint32_t khz = f % 1000;
        bool locked = hw_subghz_hop_is_locked();
        lv_snprintf(buf, sizeof(buf), "%s %lu.%03lu MHz",
                    locked ? LV_SYMBOL_WIFI : LV_SYMBOL_LOOP,
                    (unsigned long)mhz, (unsigned long)khz);
        lv_obj_set_style_text_color(freq_lbl,
                                     locked ? COLOR_GREEN : COLOR_YELLOW, 0);
    } else {
        uint32_t mhz = hw_subghz_get_frequency() / 1000;
        uint32_t khz = hw_subghz_get_frequency() % 1000;
        lv_snprintf(buf, sizeof(buf), "%lu.%03lu MHz",
                    (unsigned long)mhz, (unsigned long)khz);
        lv_obj_set_style_text_color(freq_lbl, COLOR_SECONDARY, 0);
    }
    lv_label_set_text(freq_lbl, buf);
}

static void start_listening(SubghzApp *app)
{
    displayed_count = 0;
    lv_obj_clean(list_cont);
    lv_label_set_text(count_lbl, "Listening...");

    if(app->hopping) {
        sim_subghz_start_read_hopping(read_cb, app);
    } else {
        sim_subghz_start_read(read_cb, app);
    }
    update_freq_status(app);
}

static void hop_timer_cb(lv_timer_t *t)
{
    SubghzApp *app = lv_timer_get_user_data(t);
    if(!app || !app->hopping) return;

    uint32_t f = hw_subghz_hop_current_freq();
    bool locked = hw_subghz_hop_is_locked();
    if(f == last_hop_freq && locked == last_hop_locked) return;
    last_hop_freq = f;
    last_hop_locked = locked;
    update_freq_status(app);
}

static void config_btn_cb(lv_event_t *e)
{
    SubghzApp *app = lv_event_get_user_data(e);
    scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Settings);
}

static void stop_btn_cb(lv_event_t *e)
{
    SubghzApp *app = lv_event_get_user_data(e);
    scene_manager_previous_scene(&app->scene_mgr);
}

void subghz_scene_read_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    displayed_count = 0;
    last_hop_freq = 0;
    last_hop_locked = false;

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);

    status_bar_create(view, "Read", COLOR_GREEN);

    freq_lbl = lv_label_create(view);
    lv_label_set_text(freq_lbl, "");
    lv_obj_set_style_text_color(freq_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(freq_lbl, FONT_SMALL, 0);
    lv_obj_align(freq_lbl, LV_ALIGN_TOP_MID, 0, 55);

    count_lbl = lv_label_create(view);
    lv_label_set_text(count_lbl, "Listening...");
    lv_obj_set_style_text_color(count_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(count_lbl, FONT_MONO, 0);
    lv_obj_align(count_lbl, LV_ALIGN_BOTTOM_MID, 0, -95);

    list_cont = lv_obj_create(view);
    lv_obj_set_size(list_cont, DISP_W - 40, DISP_H - 190);
    lv_obj_align(list_cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_style_pad_all(list_cont, 0, 0);
    lv_obj_set_style_pad_row(list_cont, 8, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *btn_row = lv_obj_create(view);
    lv_obj_set_size(btn_row, DISP_W - 40, 50);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 20, 0);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *cfg_btn = lv_obj_create(btn_row);
    lv_obj_set_size(cfg_btn, 130, 44);
    lv_obj_set_style_bg_color(cfg_btn, COLOR_BLUE, 0);
    lv_obj_set_style_bg_opa(cfg_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cfg_btn, 0, 0);
    lv_obj_set_style_radius(cfg_btn, 22, 0);
    lv_obj_add_flag(cfg_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(cfg_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(cfg_btn, config_btn_cb, LV_EVENT_CLICKED, app);
    lv_obj_t *cfg_lbl = lv_label_create(cfg_btn);
    lv_label_set_text(cfg_lbl, LV_SYMBOL_SETTINGS " Config");
    lv_obj_set_style_text_color(cfg_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(cfg_lbl, FONT_BODY, 0);
    lv_obj_center(cfg_lbl);

    lv_obj_t *stop_btn = lv_obj_create(btn_row);
    lv_obj_set_size(stop_btn, 130, 44);
    lv_obj_set_style_bg_color(stop_btn, COLOR_RED, 0);
    lv_obj_set_style_bg_opa(stop_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(stop_btn, 0, 0);
    lv_obj_set_style_radius(stop_btn, 22, 0);
    lv_obj_add_flag(stop_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(stop_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(stop_btn, stop_btn_cb, LV_EVENT_CLICKED, app);
    lv_obj_t *stop_lbl = lv_label_create(stop_btn);
    lv_label_set_text(stop_lbl, LV_SYMBOL_STOP " Stop");
    lv_obj_set_style_text_color(stop_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(stop_lbl, FONT_BODY, 0);
    lv_obj_center(stop_lbl);

    hop_timer = lv_timer_create(hop_timer_cb, 300, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewCustom);

    start_listening(app);
}

bool subghz_scene_read_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if(event.type == SceneEventTypeCustom &&
       event.event == SUBGHZ_EVT_READ_UPDATE) {
        rebuild_list(app);
        if(app->hopping) update_freq_status(app);
        return true;
    }
    return false;
}

void subghz_scene_read_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    sim_subghz_stop_read();
    if(hop_timer) {
        lv_timer_delete(hop_timer);
        hop_timer = NULL;
    }
    list_cont = NULL;
    count_lbl = NULL;
    freq_lbl = NULL;
    displayed_count = 0;
    view_custom_clean(app->custom);
}
