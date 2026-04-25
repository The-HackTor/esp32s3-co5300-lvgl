#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "ui/widgets/status_bar.h"
#include <string.h>

#define HISTORY_SIZE 3
#define RSSI_BAR_W   220
#define RSSI_BAR_H   14

static lv_obj_t *freq_lbl;
static lv_obj_t *mhz_lbl;
static lv_obj_t *rssi_bar_track;
static lv_obj_t *rssi_bar_fill;
static lv_obj_t *rssi_lbl;
static lv_obj_t *signal_box;
static lv_obj_t *history_lbls[HISTORY_SIZE];

static uint32_t history_freq[HISTORY_SIZE];
static uint32_t current_freq;
static float current_rssi;
static bool signal_locked;
static uint32_t last_update;

static void fmt_freq(char *buf, size_t len, uint32_t freq)
{
    if(freq > 0) {
        uint32_t mhz = freq / 1000;
        uint32_t khz = freq % 1000;
        lv_snprintf(buf, len, "%lu.%03lu", (unsigned long)mhz, (unsigned long)khz);
    } else {
        lv_snprintf(buf, len, "---.---");
    }
}

static void push_history(uint32_t freq)
{
    if(freq == 0 || history_freq[0] == freq) return;
    history_freq[2] = history_freq[1];
    history_freq[1] = history_freq[0];
    history_freq[0] = freq;
}

static void refresh_ui(void)
{
    char buf[24];

    fmt_freq(buf, sizeof(buf), current_freq);
    lv_label_set_text(freq_lbl, buf);

    if(signal_box) {
        lv_obj_set_style_bg_opa(signal_box,
                                 signal_locked ? LV_OPA_30 : LV_OPA_TRANSP, 0);
    }

    float norm = (current_rssi + 100.0f) / 70.0f;
    if(norm < 0.0f) norm = 0.0f;
    if(norm > 1.0f) norm = 1.0f;
    lv_obj_set_width(rssi_bar_fill, (int32_t)(norm * RSSI_BAR_W));

    lv_color_t c = COLOR_RED;
    if(current_rssi > -50.0f) c = COLOR_GREEN;
    else if(current_rssi > -70.0f) c = COLOR_YELLOW;
    else if(current_rssi > -85.0f) c = COLOR_ORANGE;
    lv_obj_set_style_bg_color(rssi_bar_fill, c, 0);

    lv_snprintf(buf, sizeof(buf), "%d dBm", (int)current_rssi);
    lv_label_set_text(rssi_lbl, buf);

    for(int i = 0; i < HISTORY_SIZE; i++) {
        if(!history_lbls[i]) continue;
        char hbuf[32];
        fmt_freq(buf, sizeof(buf), history_freq[i]);
        lv_snprintf(hbuf, sizeof(hbuf), "%s MHz", buf);
        lv_label_set_text(history_lbls[i], hbuf);
    }
}

static void analyzer_cb(const SimSubghzFreqResult *result, void *ctx)
{
    (void)ctx;
    uint32_t now = lv_tick_get();
    if(now - last_update < 80) return;
    last_update = now;

    current_rssi = result->rssi;

    if(result->rssi > -85.0f && result->freq_khz > 0) {
        if(!signal_locked) signal_locked = true;
        current_freq = result->freq_khz;
    } else {
        if(signal_locked) {
            push_history(current_freq);
            signal_locked = false;
        }
    }

    refresh_ui();
}

void subghz_scene_analyzer_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    last_update = 0;
    current_freq = 0;
    current_rssi = -100.0f;
    signal_locked = false;
    memset(history_freq, 0, sizeof(history_freq));

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);

    status_bar_create(view, "Freq Analyzer", COLOR_YELLOW);

    signal_box = lv_obj_create(view);
    lv_obj_set_size(signal_box, 340, 80);
    lv_obj_align(signal_box, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_color(signal_box, COLOR_GREEN, 0);
    lv_obj_set_style_bg_opa(signal_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(signal_box, 0, 0);
    lv_obj_set_style_radius(signal_box, 14, 0);
    lv_obj_remove_flag(signal_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    freq_lbl = lv_label_create(view);
    lv_label_set_text(freq_lbl, "---.---");
    lv_obj_set_style_text_color(freq_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(freq_lbl, FONT_LARGE, 0);
    lv_obj_align(freq_lbl, LV_ALIGN_TOP_MID, 0, 65);

    mhz_lbl = lv_label_create(view);
    lv_label_set_text(mhz_lbl, "MHz");
    lv_obj_set_style_text_color(mhz_lbl, COLOR_DIM, 0);
    lv_obj_set_style_text_font(mhz_lbl, FONT_BODY, 0);
    lv_obj_align(mhz_lbl, LV_ALIGN_TOP_MID, 0, 120);

    lv_obj_t *rssi_row = lv_obj_create(view);
    lv_obj_set_size(rssi_row, 360, 40);
    lv_obj_align(rssi_row, LV_ALIGN_TOP_MID, 0, 155);
    lv_obj_set_style_bg_opa(rssi_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rssi_row, 0, 0);
    lv_obj_set_style_pad_all(rssi_row, 0, 0);
    lv_obj_remove_flag(rssi_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(rssi_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rssi_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rssi_row, 12, 0);

    lv_obj_t *rssi_title = lv_label_create(rssi_row);
    lv_label_set_text(rssi_title, "RSSI");
    lv_obj_set_style_text_color(rssi_title, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(rssi_title, FONT_BODY, 0);

    rssi_bar_track = lv_obj_create(rssi_row);
    lv_obj_set_size(rssi_bar_track, RSSI_BAR_W, RSSI_BAR_H);
    lv_obj_set_style_bg_color(rssi_bar_track, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(rssi_bar_track, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rssi_bar_track, 0, 0);
    lv_obj_set_style_radius(rssi_bar_track, RSSI_BAR_H / 2, 0);
    lv_obj_set_style_pad_all(rssi_bar_track, 0, 0);
    lv_obj_remove_flag(rssi_bar_track, LV_OBJ_FLAG_SCROLLABLE);

    rssi_bar_fill = lv_obj_create(rssi_bar_track);
    lv_obj_set_size(rssi_bar_fill, 2, RSSI_BAR_H);
    lv_obj_align(rssi_bar_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(rssi_bar_fill, COLOR_RED, 0);
    lv_obj_set_style_bg_opa(rssi_bar_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rssi_bar_fill, 0, 0);
    lv_obj_set_style_radius(rssi_bar_fill, RSSI_BAR_H / 2, 0);
    lv_obj_remove_flag(rssi_bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    rssi_lbl = lv_label_create(rssi_row);
    lv_label_set_text(rssi_lbl, "-100");
    lv_obj_set_style_text_color(rssi_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(rssi_lbl, FONT_BODY, 0);

    for(int i = 0; i < HISTORY_SIZE; i++) {
        history_lbls[i] = lv_label_create(view);
        lv_label_set_text(history_lbls[i], "---.--- MHz");
        lv_obj_set_style_text_color(history_lbls[i],
                                     i == 0 ? COLOR_SECONDARY : COLOR_DIM, 0);
        lv_obj_set_style_text_font(history_lbls[i], FONT_BODY, 0);
        lv_obj_align(history_lbls[i], LV_ALIGN_CENTER, 0, 30 + i * 32);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewCustom);
    sim_subghz_start_analyzer(analyzer_cb, app);
}

bool subghz_scene_analyzer_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_analyzer_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    sim_subghz_stop_analyzer();
    freq_lbl = NULL;
    mhz_lbl = NULL;
    rssi_bar_track = NULL;
    rssi_bar_fill = NULL;
    rssi_lbl = NULL;
    signal_box = NULL;
    for(int i = 0; i < HISTORY_SIZE; i++) history_lbls[i] = NULL;
    view_custom_clean(app->custom);
}
