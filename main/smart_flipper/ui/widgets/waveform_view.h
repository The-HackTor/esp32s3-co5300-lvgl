#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <lvgl.h>
#include <stdint.h>

typedef struct {
    lv_obj_t       *canvas;
    lv_draw_buf_t  *draw_buf;
    const int32_t  *samples;
    uint16_t        sample_count;
    int32_t         scroll_us;
    int32_t         total_us;
    uint8_t         zoom_level;
    int32_t         width;
    int32_t         height;
} WaveformView;

lv_obj_t *waveform_view_create(lv_obj_t *parent, int32_t w, int32_t h);
void waveform_view_set_data(lv_obj_t *obj, const int32_t *samples,
                             uint16_t count);
void waveform_view_zoom_in(lv_obj_t *obj);
void waveform_view_zoom_out(lv_obj_t *obj);
void waveform_view_scroll_left(lv_obj_t *obj);
void waveform_view_scroll_right(lv_obj_t *obj);
void waveform_view_redraw(lv_obj_t *obj);

#endif
