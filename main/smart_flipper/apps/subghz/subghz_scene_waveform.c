#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "ui/widgets/waveform_view.h"

#define CANVAS_W    360
#define CANVAS_H    160

static lv_obj_t *wv_canvas;

static void zoom_in_cb(lv_event_t *e)
{
    (void)e;
    waveform_view_zoom_in(wv_canvas);
}

static void zoom_out_cb(lv_event_t *e)
{
    (void)e;
    waveform_view_zoom_out(wv_canvas);
}

static void scroll_left_cb(lv_event_t *e)
{
    (void)e;
    waveform_view_scroll_left(wv_canvas);
}

static void scroll_right_cb(lv_event_t *e)
{
    (void)e;
    waveform_view_scroll_right(wv_canvas);
}

static lv_obj_t *create_ctrl_btn(lv_obj_t *parent, const char *text,
                                  lv_event_cb_t cb, void *ud)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, 56, 40);
    lv_obj_set_style_bg_color(btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ud);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_center(lbl);

    return btn;
}

void subghz_scene_waveform_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    lv_obj_t *view = view_custom_get_view(app->custom);
    view_custom_clean(app->custom);

    lv_obj_t *title = lv_label_create(view);
    lv_label_set_text(title, "Waveform");
    lv_obj_set_style_text_font(title, FONT_TITLE, 0);
    lv_obj_set_style_text_color(title, COLOR_GREEN, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 32);

    if(app->decoded_valid) {
        char info[48];
        lv_snprintf(info, sizeof(info), "%s  0x%lX  %ub",
                    app->last_decoded.protocol,
                    (unsigned long)app->last_decoded.data,
                    app->last_decoded.bits);
        lv_obj_t *dec_lbl = lv_label_create(view);
        lv_label_set_text(dec_lbl, info);
        lv_obj_set_style_text_color(dec_lbl, COLOR_GREEN, 0);
        lv_obj_set_style_text_font(dec_lbl, FONT_BODY, 0);
        lv_obj_align(dec_lbl, LV_ALIGN_TOP_MID, 0, 62);
    }

    /* CO5300 is round; canvas height capped to fit visible inscribed area */
    wv_canvas = waveform_view_create(view, CANVAS_W, CANVAS_H);
    lv_obj_align(wv_canvas, LV_ALIGN_CENTER, 0, -10);

    if(app->raw_valid && app->raw.count > 0) {
        waveform_view_set_data(wv_canvas, app->raw.samples, app->raw.count);
    }

    lv_obj_t *ctrl_row = lv_obj_create(view);
    lv_obj_set_size(ctrl_row, 320, 52);
    lv_obj_align(ctrl_row, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_obj_set_style_bg_opa(ctrl_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctrl_row, 0, 0);
    lv_obj_set_style_pad_all(ctrl_row, 0, 0);
    lv_obj_set_style_pad_column(ctrl_row, 16, 0);
    lv_obj_remove_flag(ctrl_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(ctrl_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_ctrl_btn(ctrl_row, LV_SYMBOL_LEFT,  scroll_left_cb,  NULL);
    create_ctrl_btn(ctrl_row, LV_SYMBOL_MINUS, zoom_out_cb,     NULL);
    create_ctrl_btn(ctrl_row, LV_SYMBOL_PLUS,  zoom_in_cb,      NULL);
    create_ctrl_btn(ctrl_row, LV_SYMBOL_RIGHT, scroll_right_cb, NULL);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewCustom);
}

bool subghz_scene_waveform_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_waveform_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    wv_canvas = NULL;
    view_custom_clean(app->custom);
}
