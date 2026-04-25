#include "waveform_view.h"
#include "ui/styles.h"
#include <string.h>

#define ZOOM_LEVELS     7
#define WAVE_HIGH_Y     8
#define WAVE_LOW_Y_OFF  4
#define MARGIN_TOP      20
#define TIME_AXIS_H     16

static const int32_t us_per_px[ZOOM_LEVELS] = {
    1, 5, 10, 50, 100, 500, 1000
};

static const char *zoom_labels[ZOOM_LEVELS] = {
    "1us/px", "5us/px", "10us/px", "50us/px",
    "100us/px", "500us/px", "1ms/px"
};

static int32_t compute_total_us(const int32_t *samples, uint16_t count)
{
    int32_t total = 0;
    for (uint16_t i = 0; i < count; i++) {
        int32_t s = samples[i];
        total += (s > 0) ? s : -s;
    }
    return total;
}

static void draw_waveform(WaveformView *wv)
{
    int32_t w = wv->width;
    int32_t h = wv->height;
    int32_t scale = us_per_px[wv->zoom_level];
    int32_t viewport_us = w * scale;

    lv_canvas_fill_bg(wv->canvas, COLOR_CARD_BG, LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(wv->canvas, &layer);

    int32_t wave_top = MARGIN_TOP;
    int32_t wave_h = h - MARGIN_TOP - TIME_AXIS_H;
    int32_t mid_y = wave_top + wave_h / 2;
    int32_t high_h = wave_h / 2 - 4;
    int32_t low_h = 2;

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(0x333333);
    line_dsc.width = 1;
    line_dsc.p1.x = 0;
    line_dsc.p1.y = mid_y;
    line_dsc.p2.x = w - 1;
    line_dsc.p2.y = mid_y;
    lv_draw_line(&layer, &line_dsc);

    int32_t acc_us = 0;
    int32_t scroll_end = wv->scroll_us + viewport_us;
    int32_t prev_x = -1;
    bool prev_level = false;

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 0;
    rect_dsc.border_width = 0;
    rect_dsc.bg_opa = LV_OPA_COVER;

    for (uint16_t i = 0; i < wv->sample_count; i++) {
        int32_t s = wv->samples[i];
        bool level = s > 0;
        int32_t dur = level ? s : -s;
        int32_t sample_start = acc_us;
        int32_t sample_end = acc_us + dur;
        acc_us = sample_end;

        if (sample_end <= wv->scroll_us) continue;
        if (sample_start >= scroll_end) break;

        int32_t vis_start = (sample_start < wv->scroll_us)
                            ? wv->scroll_us : sample_start;
        int32_t vis_end = (sample_end > scroll_end)
                          ? scroll_end : sample_end;

        int32_t px_start = (vis_start - wv->scroll_us) / scale;
        int32_t px_end = (vis_end - wv->scroll_us) / scale;
        if (px_end <= px_start) px_end = px_start + 1;
        if (px_end > w) px_end = w;

        if (prev_x >= 0 && prev_level != level && px_start > 0) {
            lv_draw_line_dsc_t edge;
            lv_draw_line_dsc_init(&edge);
            edge.color = COLOR_GREEN;
            edge.width = 1;
            edge.p1.x = px_start;
            edge.p1.y = mid_y - high_h;
            edge.p2.x = px_start;
            edge.p2.y = mid_y + 4;
            lv_draw_line(&layer, &edge);
        }

        lv_area_t area;
        if (level) {
            rect_dsc.bg_color = COLOR_GREEN;
            area.x1 = px_start;
            area.y1 = mid_y - high_h;
            area.x2 = px_end - 1;
            area.y2 = mid_y - 2;
        } else {
            rect_dsc.bg_color = COLOR_DIM;
            area.x1 = px_start;
            area.y1 = mid_y + 2;
            area.x2 = px_end - 1;
            area.y2 = mid_y + 2 + low_h;
        }
        lv_draw_rect(&layer, &rect_dsc, &area);

        prev_x = px_end;
        prev_level = level;
    }

    int32_t grid_us;
    if (scale <= 5) grid_us = 100;
    else if (scale <= 50) grid_us = 1000;
    else if (scale <= 500) grid_us = 10000;
    else grid_us = 50000;

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = COLOR_SECONDARY;
    label_dsc.font = FONT_MONO;

    int32_t first_mark = ((wv->scroll_us / grid_us) + 1) * grid_us;
    for (int32_t t = first_mark; t < scroll_end; t += grid_us) {
        int32_t px = (t - wv->scroll_us) / scale;
        if (px < 0 || px >= w) continue;

        lv_draw_line_dsc_t tick;
        lv_draw_line_dsc_init(&tick);
        tick.color = lv_color_hex(0x333333);
        tick.width = 1;
        tick.p1.x = px;
        tick.p1.y = h - TIME_AXIS_H;
        tick.p2.x = px;
        tick.p2.y = h - TIME_AXIS_H + 4;
        lv_draw_line(&layer, &tick);

        char tbuf[16];
        if (t >= 1000000) {
            lv_snprintf(tbuf, sizeof(tbuf), "%lums",
                        (unsigned long)(t / 1000));
        } else if (t >= 1000) {
            int32_t ms = t / 1000;
            int32_t frac = (t % 1000) / 100;
            if (frac) {
                lv_snprintf(tbuf, sizeof(tbuf), "%ld.%ldms",
                            (long)ms, (long)frac);
            } else {
                lv_snprintf(tbuf, sizeof(tbuf), "%ldms", (long)ms);
            }
        } else {
            lv_snprintf(tbuf, sizeof(tbuf), "%ldus", (long)t);
        }

        lv_area_t ta = { .x1 = px - 20, .y1 = h - TIME_AXIS_H + 4,
                         .x2 = px + 40, .y2 = h - 1 };
        label_dsc.text = tbuf;
        lv_draw_label(&layer, &label_dsc, &ta);
    }

    lv_area_t zoom_area = { .x1 = 4, .y1 = 2, .x2 = 120, .y2 = 14 };
    label_dsc.color = COLOR_YELLOW;
    label_dsc.text = zoom_labels[wv->zoom_level];
    lv_draw_label(&layer, &label_dsc, &zoom_area);

    char pos_buf[24];
    int32_t center_us = wv->scroll_us + viewport_us / 2;
    if (center_us >= 1000) {
        lv_snprintf(pos_buf, sizeof(pos_buf), "%ld.%ldms / %ld.%ldms",
                    (long)(center_us / 1000),
                    (long)((center_us % 1000) / 100),
                    (long)(wv->total_us / 1000),
                    (long)((wv->total_us % 1000) / 100));
    } else {
        lv_snprintf(pos_buf, sizeof(pos_buf), "%ldus / %ldus",
                    (long)center_us, (long)wv->total_us);
    }
    lv_area_t pos_area = { .x1 = w - 160, .y1 = 2, .x2 = w - 4, .y2 = 14 };
    label_dsc.color = COLOR_SECONDARY;
    label_dsc.text = pos_buf;
    lv_draw_label(&layer, &label_dsc, &pos_area);

    lv_canvas_finish_layer(wv->canvas, &layer);
}

static void waveform_view_delete_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (wv) {
        if (wv->draw_buf) {
            lv_draw_buf_destroy(wv->draw_buf);
        }
        lv_free(wv);
        lv_obj_set_user_data(obj, NULL);
    }
}

lv_obj_t *waveform_view_create(lv_obj_t *parent, int32_t w, int32_t h)
{
    WaveformView *wv = lv_malloc(sizeof(WaveformView));
    if (!wv) return NULL;
    memset(wv, 0, sizeof(*wv));
    wv->width = w;
    wv->height = h;
    wv->zoom_level = 3;
    wv->scroll_us = 0;

    wv->draw_buf = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, 0);

    wv->canvas = lv_canvas_create(parent);
    lv_canvas_set_draw_buf(wv->canvas, wv->draw_buf);
    lv_obj_set_size(wv->canvas, w, h);
    lv_obj_set_user_data(wv->canvas, wv);
    lv_obj_add_event_cb(wv->canvas, waveform_view_delete_cb,
                        LV_EVENT_DELETE, NULL);

    lv_canvas_fill_bg(wv->canvas, COLOR_CARD_BG, LV_OPA_COVER);

    return wv->canvas;
}

void waveform_view_set_data(lv_obj_t *obj, const int32_t *samples,
                             uint16_t count)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (!wv) return;
    wv->samples = samples;
    wv->sample_count = count;
    wv->scroll_us = 0;
    wv->total_us = compute_total_us(samples, count);
    draw_waveform(wv);
}

void waveform_view_zoom_in(lv_obj_t *obj)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (!wv || wv->zoom_level == 0) return;

    int32_t center = wv->scroll_us +
                     (wv->width * us_per_px[wv->zoom_level]) / 2;
    wv->zoom_level--;
    int32_t new_viewport = wv->width * us_per_px[wv->zoom_level];
    wv->scroll_us = center - new_viewport / 2;
    if (wv->scroll_us < 0) wv->scroll_us = 0;

    draw_waveform(wv);
}

void waveform_view_zoom_out(lv_obj_t *obj)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (!wv || wv->zoom_level >= ZOOM_LEVELS - 1) return;

    int32_t center = wv->scroll_us +
                     (wv->width * us_per_px[wv->zoom_level]) / 2;
    wv->zoom_level++;
    int32_t new_viewport = wv->width * us_per_px[wv->zoom_level];
    wv->scroll_us = center - new_viewport / 2;
    if (wv->scroll_us < 0) wv->scroll_us = 0;
    if (wv->scroll_us + new_viewport > wv->total_us)
        wv->scroll_us = wv->total_us - new_viewport;
    if (wv->scroll_us < 0) wv->scroll_us = 0;

    draw_waveform(wv);
}

void waveform_view_scroll_left(lv_obj_t *obj)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (!wv) return;
    int32_t step = wv->width * us_per_px[wv->zoom_level] / 4;
    wv->scroll_us -= step;
    if (wv->scroll_us < 0) wv->scroll_us = 0;
    draw_waveform(wv);
}

void waveform_view_scroll_right(lv_obj_t *obj)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (!wv) return;
    int32_t step = wv->width * us_per_px[wv->zoom_level] / 4;
    int32_t viewport = wv->width * us_per_px[wv->zoom_level];
    wv->scroll_us += step;
    if (wv->scroll_us + viewport > wv->total_us)
        wv->scroll_us = wv->total_us - viewport;
    if (wv->scroll_us < 0) wv->scroll_us = 0;
    draw_waveform(wv);
}

void waveform_view_redraw(lv_obj_t *obj)
{
    WaveformView *wv = lv_obj_get_user_data(obj);
    if (wv) draw_waveform(wv);
}
