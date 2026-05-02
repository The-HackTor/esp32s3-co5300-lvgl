#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "ui/widgets/status_bar.h"
#include <string.h>

#define RSSI_HISTORY_SIZE 100
#define RSSI_MIN          -90.0f
#define RSSI_RANGE        60.0f

#define CANVAS_W    240
#define CANVAS_H    100
#define BAR_W       3
#define BAR_GAP     1
#define BAR_STRIDE  (BAR_W + BAR_GAP)
#define MAX_BARS    (CANVAS_W / BAR_STRIDE)

static const char *mod_labels[] = {"AM270", "AM650", "FM2.38", "FM47.6"};

static uint8_t rssi_history[RSSI_HISTORY_SIZE];
static uint8_t rssi_idx;
static bool rssi_wrapped;
static uint32_t last_sample_count;

static lv_obj_t *freq_lbl;
static lv_obj_t *rssi_value_lbl;
static lv_obj_t *sample_lbl;
static lv_obj_t *canvas;
static lv_timer_t *rssi_timer;
static lv_obj_t *rec_btn;
static lv_obj_t *rec_btn_lbl;
static bool recording;

static uint16_t canvas_buf[CANVAS_W * CANVAS_H];

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint16_t)(r >> 3) << 11) |
           ((uint16_t)(g >> 2) << 5)  |
           (b >> 3);
}

static const uint16_t BG_COLOR   = 0x0841;

static uint8_t rssi_to_bar(float rssi)
{
    if(rssi <= RSSI_MIN) return 0;
    float norm = (rssi - RSSI_MIN) / RSSI_RANGE;
    if(norm > 1.0f) norm = 1.0f;
    return (uint8_t)(norm * CANVAS_H);
}

static uint16_t bar_color_16(uint8_t h)
{
    if(h > (uint8_t)(CANVAS_H * 0.65f)) return rgb565(0x00, 0xFF, 0x00);
    if(h > (uint8_t)(CANVAS_H * 0.35f)) return rgb565(0xFF, 0xFF, 0x00);
    if(h > (uint8_t)(CANVAS_H * 0.15f)) return rgb565(0xFF, 0xA0, 0x00);
    return rgb565(0xFF, 0x30, 0x30);
}

static void redraw_canvas(void)
{
    if(!canvas) return;

    for(int i = 0; i < CANVAS_W * CANVAS_H; i++)
        canvas_buf[i] = BG_COLOR;

    int total = rssi_wrapped ? RSSI_HISTORY_SIZE : rssi_idx;
    int start = rssi_wrapped ? rssi_idx : 0;
    int draw_count = total < MAX_BARS ? total : MAX_BARS;
    int skip = total > MAX_BARS ? total - MAX_BARS : 0;

    for(int i = 0; i < draw_count; i++) {
        int idx = (start + skip + i) % RSSI_HISTORY_SIZE;
        uint8_t h = rssi_history[idx];
        if(h < 2) h = 2;
        uint16_t c16 = bar_color_16(h);
        int x0 = i * BAR_STRIDE;
        int y_top = CANVAS_H - h;

        for(int bx = 0; bx < BAR_W && (x0 + bx) < CANVAS_W; bx++) {
            for(int by = y_top; by < CANVAS_H; by++) {
                canvas_buf[by * CANVAS_W + x0 + bx] = c16;
            }
        }
    }

    lv_obj_invalidate(canvas);
}

static void draw_tick_bar(uint8_t h)
{
    if(!canvas) return;

    int col = rssi_idx % MAX_BARS;
    int x0 = col * BAR_STRIDE;
    if(h < 2) h = 2;
    uint8_t r, g, b;
    if(h > (uint8_t)(CANVAS_H * 0.65f))      { r = 0x00; g = 0xFF; b = 0x00; }
    else if(h > (uint8_t)(CANVAS_H * 0.35f)) { r = 0xFF; g = 0xFF; b = 0x00; }
    else if(h > (uint8_t)(CANVAS_H * 0.15f)) { r = 0xFF; g = 0xA0; b = 0x00; }
    else                                      { r = 0xFF; g = 0x30; b = 0x30; }
    lv_color_t bar_c = lv_color_make(r, g, b);
    lv_color_t bg_c = lv_color_hex(0x0A0A0A);
    int y_top = CANVAS_H - h;

    for(int bx = 0; bx < BAR_W && (x0 + bx) < CANVAS_W; bx++) {
        for(int by = 0; by < CANVAS_H; by++) {
            lv_canvas_set_px(canvas, x0 + bx, by,
                             (by >= y_top) ? bar_c : bg_c, LV_OPA_COVER);
        }
    }
}

static void update_freq_label(SubghzApp *app)
{
    if(!freq_lbl) return;
    char buf[48];
    uint32_t mhz = app->frequency / 1000;
    uint32_t khz = app->frequency % 1000;
    lv_snprintf(buf, sizeof(buf), "%lu.%03lu MHz  %s",
                (unsigned long)mhz, (unsigned long)khz,
                mod_labels[app->preset % 4]);
    lv_label_set_text(freq_lbl, buf);
}

static void rssi_timer_cb(lv_timer_t *t)
{
    SubghzApp *app = lv_timer_get_user_data(t);
    if(!app) return;

    if(recording) {
        uint32_t count = hw_subghz_capture_sample_count();
        uint32_t delta = count >= last_sample_count
                       ? count - last_sample_count
                       : 0;
        last_sample_count = count;

        uint8_t h;
        if(delta == 0) h = 2;
        else if(delta > 200) h = CANVAS_H;
        else h = (uint8_t)((uint32_t)delta * CANVAS_H / 200);

        draw_tick_bar(h);
        rssi_idx++;
        if(rssi_idx >= RSSI_HISTORY_SIZE) rssi_idx = 0;

        if(sample_lbl) {
            char buf[32];
            lv_snprintf(buf, sizeof(buf), "%u samples", (unsigned)count);
            lv_obj_set_style_text_color(sample_lbl, COLOR_SECONDARY, 0);
            lv_label_set_text(sample_lbl, buf);
        }
        return;
    }

    float rssi = hw_subghz_get_live_rssi();
    uint8_t h = rssi_to_bar(rssi);

    rssi_history[rssi_idx] = h;
    rssi_idx++;
    if(rssi_idx >= RSSI_HISTORY_SIZE) {
        rssi_idx = 0;
        rssi_wrapped = true;
    }

    redraw_canvas();

    if(rssi_value_lbl) {
        char rbuf[16];
        lv_snprintf(rbuf, sizeof(rbuf), "%d dBm", (int)rssi);
        lv_label_set_text(rssi_value_lbl, rbuf);
        lv_color_t c = lv_color_hex(0xFF3030);
        if(rssi > -60.0f) c = lv_color_hex(0x00FF00);
        else if(rssi > -75.0f) c = lv_color_hex(0xFFFF00);
        else if(rssi > -85.0f) c = lv_color_hex(0xFFA000);
        lv_obj_set_style_text_color(rssi_value_lbl, c, 0);
    }
}

static void capture_done_cb(bool success, const SimSubghzRaw *raw, void *ctx)
{
    SubghzApp *app = ctx;
    if(success && raw) {
        memcpy(&app->raw, raw, sizeof(SimSubghzRaw));
        app->raw_valid = true;

        const SimSubghzDecoded *dec = hw_subghz_capture_decoded();
        if(dec) {
            app->last_decoded = *dec;
            app->decoded_valid = true;
        }
    }
    scene_manager_handle_custom_event(&app->scene_mgr, SUBGHZ_EVT_CAPTURE_DONE);
}

static void rec_btn_cb(lv_event_t *e)
{
    SubghzApp *app = lv_event_get_user_data(e);

    if(!recording) {
        recording = true;
        last_sample_count = 0;
        rssi_idx = 0;
        rssi_wrapped = false;
        memset(canvas_buf, 0, sizeof(canvas_buf));
        if(canvas) lv_obj_invalidate(canvas);
        lv_label_set_text(rec_btn_lbl, LV_SYMBOL_STOP " Stop");
        lv_obj_set_style_bg_color(rec_btn, lv_color_hex(0xFF3030), 0);
        if(sample_lbl) {
            lv_label_set_text(sample_lbl, "0 samples");
            lv_obj_remove_flag(sample_lbl, LV_OBJ_FLAG_HIDDEN);
        }
        sim_subghz_start_capture_continuous(app->rssi_threshold,
                                            capture_done_cb, app);
    } else {
        recording = false;
        lv_label_set_text(rec_btn_lbl, LV_SYMBOL_AUDIO " REC");
        lv_obj_set_style_bg_color(rec_btn, COLOR_CYAN, 0);
        sim_subghz_stop_capture();
    }
}

static void config_btn_cb(lv_event_t *e)
{
    SubghzApp *app = lv_event_get_user_data(e);
    recording = false;
    scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Settings);
}

void subghz_scene_capture_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    recording = false;
    rssi_idx = 0;
    rssi_wrapped = false;
    memset(rssi_history, 0, sizeof(rssi_history));
    memset(canvas_buf, 0, sizeof(canvas_buf));

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);

    status_bar_create(view, "Read RAW", COLOR_CYAN);

    freq_lbl = lv_label_create(view);
    lv_obj_set_style_text_color(freq_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(freq_lbl, FONT_SMALL, 0);
    lv_obj_align(freq_lbl, LV_ALIGN_TOP_MID, -30, 55);
    update_freq_label(app);

    rssi_value_lbl = lv_label_create(view);
    lv_label_set_text(rssi_value_lbl, "");
    lv_obj_set_style_text_color(rssi_value_lbl, COLOR_DIM, 0);
    lv_obj_set_style_text_font(rssi_value_lbl, FONT_SMALL, 0);
    lv_obj_align_to(rssi_value_lbl, freq_lbl, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    canvas = lv_canvas_create(view);
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_W, CANVAS_H,
                         LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(canvas, lv_color_hex(0x0A0A0A), LV_OPA_COVER);
    lv_obj_align(canvas, LV_ALIGN_CENTER, 0, -10);

    sample_lbl = lv_label_create(view);
    lv_label_set_text(sample_lbl, "");
    lv_obj_set_style_text_color(sample_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_font(sample_lbl, FONT_MONO, 0);
    lv_obj_align_to(sample_lbl, canvas, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
    lv_obj_add_flag(sample_lbl, LV_OBJ_FLAG_HIDDEN);

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

    rec_btn = lv_obj_create(btn_row);
    lv_obj_set_size(rec_btn, 130, 44);
    lv_obj_set_style_bg_color(rec_btn, COLOR_CYAN, 0);
    lv_obj_set_style_bg_opa(rec_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rec_btn, 0, 0);
    lv_obj_set_style_radius(rec_btn, 22, 0);
    lv_obj_add_flag(rec_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(rec_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(rec_btn, rec_btn_cb, LV_EVENT_CLICKED, app);
    rec_btn_lbl = lv_label_create(rec_btn);
    lv_label_set_text(rec_btn_lbl, LV_SYMBOL_AUDIO " REC");
    lv_obj_set_style_text_color(rec_btn_lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(rec_btn_lbl, FONT_BODY, 0);
    lv_obj_center(rec_btn_lbl);

    rssi_timer = lv_timer_create(rssi_timer_cb, 150, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewCustom);
}

bool subghz_scene_capture_on_event(void *ctx, SceneEvent event)
{
    if(event.type == SceneEventTypeCustom &&
       event.event == SUBGHZ_EVT_CAPTURE_DONE) {
        SubghzApp *app = ctx;
        recording = false;

        if(rec_btn_lbl) {
            lv_label_set_text(rec_btn_lbl, LV_SYMBOL_AUDIO " REC");
            lv_obj_set_style_bg_color(rec_btn, COLOR_CYAN, 0);
        }

        if(!app->raw_valid || app->raw.count == 0) {
            if(sample_lbl) {
                lv_label_set_text(sample_lbl, "No signal detected");
                lv_obj_remove_flag(sample_lbl, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_text_color(sample_lbl, lv_color_hex(0xFF3030), 0);
            }
            return true;
        }

        scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_CaptureResult);
        return true;
    }
    return false;
}

void subghz_scene_capture_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    recording = false;
    hw_subghz_stop_capture();
    if(rssi_timer) {
        lv_timer_delete(rssi_timer);
        rssi_timer = NULL;
    }
    freq_lbl = NULL;
    rssi_value_lbl = NULL;
    canvas = NULL;
    sample_lbl = NULL;
    rec_btn = NULL;
    rec_btn_lbl = NULL;
    view_custom_clean(app->custom);
}
