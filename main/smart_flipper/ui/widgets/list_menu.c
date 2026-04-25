#include "list_menu.h"
#include "ui/styles.h"

typedef struct {
    list_menu_cb_t cb;
    void          *user_data;
    uint32_t       index;
} list_item_ctx_t;

static void item_clicked(lv_event_t *e)
{
    list_item_ctx_t *ctx = lv_event_get_user_data(e);
    if(ctx && ctx->cb) ctx->cb(ctx->index, ctx->user_data);
}

static void item_delete(lv_event_t *e)
{
    list_item_ctx_t *ctx = lv_event_get_user_data(e);
    if(ctx) lv_free(ctx);
}

lv_obj_t *list_menu_create(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, DISP_W - 50, DISP_H - 110);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 8, 0);
    lv_obj_set_style_pad_top(cont, 8, 0);
    lv_obj_set_style_pad_bottom(cont, 40, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    return cont;
}

void list_menu_add_item(lv_obj_t *list, const char *icon, const char *text,
                        lv_color_t color, list_menu_cb_t cb, void *user_data)
{
    uint32_t idx = lv_obj_get_child_count(list);

    list_item_ctx_t *ctx = lv_malloc(sizeof(list_item_ctx_t));
    ctx->cb = cb;
    ctx->user_data = user_data;
    ctx->index = idx;

    lv_obj_t *btn = lv_obj_create(list);
    lv_obj_add_style(btn, &style_list_btn, 0);
    lv_obj_set_height(btn, 56);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, item_clicked, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(btn, item_delete, LV_EVENT_DELETE, ctx);

    lv_obj_t *icn = lv_label_create(btn);
    lv_label_set_text(icn, icon);
    lv_obj_set_style_text_color(icn, color, 0);
    lv_obj_set_style_text_font(icn, FONT_MEDIUM, 0);
    lv_obj_align(icn, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 36, 0);
}

void list_menu_clear(lv_obj_t *list)
{
    lv_obj_clean(list);
}

void list_menu_refresh(lv_obj_t *list)
{
    (void)list;
}
