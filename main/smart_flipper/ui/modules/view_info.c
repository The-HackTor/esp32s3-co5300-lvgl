#include "view_info.h"
#include "ui/styles.h"
#include "ui/circular_layout.h"
#include "ui/widgets/back_button.h"
#include <stdlib.h>

typedef struct {
    ViewInfoButtonCb cb;
    void            *ctx;
} BtnCtx;

struct ViewInfo {
    lv_obj_t *root;
    lv_obj_t *header_bar;
    lv_obj_t *title_lbl;
    lv_obj_t *scroll;     /* Scrollable content area */
    BtnCtx    buttons[VIEW_INFO_MAX_BUTTONS];
    uint32_t  btn_count;
};

static void btn_clicked(lv_event_t *e)
{
    BtnCtx *bctx = lv_event_get_user_data(e);
    if(bctx && bctx->cb) bctx->cb(bctx->ctx);
}

/* --- ViewModule vtable --- */
static lv_obj_t *info_get_view(void *m) { return ((ViewInfo *)m)->root; }

static void info_reset(void *m)
{
    ViewInfo *info = m;
    lv_obj_clean(info->scroll);
    lv_label_set_text(info->title_lbl, "");
    info->btn_count = 0;
}

static void info_destroy(void *m)
{
    ViewInfo *info = m;
    if(info->root) { lv_obj_delete(info->root); info->root = NULL; }
    free(info);
}

static const ViewModuleVtable info_vtable = {
    .get_view = info_get_view,
    .reset    = info_reset,
    .destroy  = info_destroy,
};

ViewInfo *view_info_alloc(lv_obj_t *parent)
{
    ViewInfo *info = calloc(1, sizeof(ViewInfo));

    info->root = lv_obj_create(parent);
    lv_obj_add_flag(info->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(info->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(info->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(info->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(info->root, 0, 0);
    lv_obj_set_style_pad_all(info->root, 0, 0);
    lv_obj_remove_flag(info->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Header bar */
    info->header_bar = lv_obj_create(info->root);
    lv_obj_set_size(info->header_bar, DISP_W, 80);
    lv_obj_align(info->header_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(info->header_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info->header_bar, 0, 0);
    lv_obj_remove_flag(info->header_bar, LV_OBJ_FLAG_SCROLLABLE);

    back_button_create(info->header_bar);

    info->title_lbl = lv_label_create(info->header_bar);
    lv_label_set_text(info->title_lbl, "");
    lv_obj_set_style_text_font(info->title_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(info->title_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(info->title_lbl, LV_ALIGN_TOP_MID, 0, 24);

    /* Scrollable content area -- narrower to fit inside circular display */
    info->scroll = lv_obj_create(info->root);
    lv_obj_set_size(info->scroll, DISP_W - 80, DISP_H - 80);
    lv_obj_align(info->scroll, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_opa(info->scroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(info->scroll, 0, 0);
    lv_obj_set_style_pad_all(info->scroll, 4, 0);
    lv_obj_set_layout(info->scroll, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info->scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info->scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(info->scroll, 6, 0);
    lv_obj_set_scrollbar_mode(info->scroll, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(info->scroll, LV_OBJ_FLAG_SCROLL_MOMENTUM);

    return info;
}

void view_info_free(ViewInfo *info) { info_destroy(info); }
ViewModule view_info_get_module(ViewInfo *info)
{
    return (ViewModule){ .module = info, .vtable = &info_vtable };
}
lv_obj_t *view_info_get_view(ViewInfo *info) { return info->root; }
void view_info_reset(ViewInfo *info) { info_reset(info); }

void view_info_set_header(ViewInfo *info, const char *title, lv_color_t accent)
{
    lv_label_set_text(info->title_lbl, title);
    lv_obj_set_style_text_color(info->title_lbl, accent, 0);
}

void view_info_add_field(ViewInfo *info, const char *key, const char *value, lv_color_t color)
{
    lv_obj_t *row = lv_obj_create(info->scroll);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_set_style_pad_hor(row, 4, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *klbl = lv_label_create(row);
    lv_label_set_text(klbl, key);
    lv_obj_set_style_text_font(klbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(klbl, COLOR_SECONDARY, 0);
    lv_obj_align(klbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *vlbl = lv_label_create(row);
    lv_label_set_text(vlbl, value);
    lv_obj_set_style_text_font(vlbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(vlbl, color, 0);
    lv_obj_align(vlbl, LV_ALIGN_RIGHT_MID, 0, 0);
}

void view_info_add_separator(ViewInfo *info)
{
    lv_obj_t *line = lv_obj_create(info->scroll);
    lv_obj_set_size(line, LV_PCT(90), 1);
    lv_obj_set_style_bg_color(line, COLOR_DIM, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

void view_info_add_text_block(ViewInfo *info, const char *text,
                              const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *card = lv_obj_create(info->scroll);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
}

void view_info_add_pill(ViewInfo *info, const char *text, lv_color_t color)
{
    lv_obj_t *pill = lv_obj_create(info->scroll);
    lv_obj_set_size(pill, LV_SIZE_CONTENT, 36);
    lv_obj_set_style_bg_color(pill, color, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill, 18, 0);
    lv_obj_set_style_border_width(pill, 0, 0);
    lv_obj_set_style_pad_hor(pill, 18, 0);
    lv_obj_set_style_pad_ver(pill, 0, 0);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(pill);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, COLOR_BG, 0);
    lv_obj_center(lbl);
}

void view_info_add_waveform(ViewInfo *info, const uint16_t *timings,
                            size_t n_timings, lv_color_t color)
{
    lv_obj_t *row = lv_obj_create(info->scroll);
    lv_obj_set_size(row, LV_PCT(100), 64);
    lv_obj_set_style_bg_color(row, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_column(row, 1, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if(!timings || n_timings == 0) return;

    const size_t cap = n_timings < 64 ? n_timings : 64;
    uint64_t total = 0;
    for(size_t i = 0; i < cap; i++) total += timings[i];
    if(total == 0) return;

    const int32_t avail_w = DISP_W - 80 - 24;
    const int32_t mark_h  = 48;
    const int32_t space_h = 4;

    for(size_t i = 0; i < cap; i++) {
        const bool is_mark = (i & 1) == 0;
        int32_t w = (int32_t)(((uint64_t)timings[i] * (uint64_t)avail_w) / total);
        if(w < 1) w = 1;

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, w, is_mark ? mark_h : space_h);
        lv_obj_set_style_bg_color(bar, is_mark ? color : COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
}

void view_info_add_button(ViewInfo *info, const char *text, lv_color_t color,
                          ViewInfoButtonCb cb, void *ctx)
{
    if(info->btn_count >= VIEW_INFO_MAX_BUTTONS) return;

    BtnCtx *bctx = &info->buttons[info->btn_count++];
    bctx->cb = cb;
    bctx->ctx = ctx;

    lv_obj_t *btn = lv_button_create(info->scroll);
    lv_obj_set_size(btn, 180, 56);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_set_style_radius(btn, 28, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, btn_clicked, LV_EVENT_CLICKED, bctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_MENU, 0);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_center(lbl);
}

static lv_obj_t *make_row_btn(lv_obj_t *parent, const char *text,
                               lv_color_t bg, lv_color_t fg,
                               ViewInfo *info, ViewInfoButtonCb cb, void *ctx)
{
    if(info->btn_count >= VIEW_INFO_MAX_BUTTONS) return NULL;
    BtnCtx *bctx = &info->buttons[info->btn_count++];
    bctx->cb = cb;
    bctx->ctx = ctx;

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_height(btn, 48);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, 24, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, btn_clicked, LV_EVENT_CLICKED, bctx);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, fg, 0);
    lv_obj_center(lbl);
    return btn;
}

void view_info_add_button_row(ViewInfo *info,
                              const char *text1, lv_color_t color1,
                              ViewInfoButtonCb cb1, void *ctx1,
                              const char *text2, lv_color_t color2,
                              ViewInfoButtonCb cb2, void *ctx2)
{
    lv_obj_t *row = lv_obj_create(info->scroll);
    lv_obj_set_size(row, LV_PCT(100), 56);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    make_row_btn(row, text1, color1, COLOR_PRIMARY, info, cb1, ctx1);
    make_row_btn(row, text2, color2, COLOR_BG, info, cb2, ctx2);
}
