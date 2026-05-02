#include "view_submenu.h"
#include "ui/styles.h"
#include "ui/circular_layout.h"
#include "ui/widgets/back_button.h"
#include <stdlib.h>

typedef struct {
    ViewSubmenuCallback   cb;
    ViewSubmenuCallback   long_press_cb;
    ViewSubmenuPressCb    on_press;
    ViewSubmenuReleaseCb  on_release;
    void                 *ctx;
    uint32_t              index;
} ItemCtx;

struct ViewSubmenu {
    lv_obj_t *root;
    lv_obj_t *header_bar;
    lv_obj_t *title_lbl;
    lv_obj_t *list;
    ItemCtx  *items;
    uint32_t  item_count;
    uint32_t  item_cap;
};

static void apply_barrel(lv_obj_t *list)
{
    int32_t center_y = DISP_H / 2;
    uint32_t count = lv_obj_get_child_count(list);

    for(uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(list, i);
        lv_area_t a;
        lv_obj_get_coords(child, &a);

        int32_t dy = (a.y1 + a.y2) / 2 - center_y;

        lv_obj_set_width(child, circular_safe_width(dy, 200));

        int32_t opa = circular_curve_factor(dy);
        if(opa < 80) opa = 80;
        lv_obj_set_style_opa(child, opa, 0);
    }
}

static void scroll_cb(lv_event_t *e)
{
    apply_barrel(lv_event_get_target(e));
}

static void item_clicked(lv_event_t *e)
{
    ItemCtx *ctx = lv_event_get_user_data(e);
    if(ctx && ctx->cb) ctx->cb(ctx->ctx, ctx->index);
}

static void item_long_pressed(lv_event_t *e)
{
    ItemCtx *ctx = lv_event_get_user_data(e);
    if(ctx && ctx->long_press_cb) ctx->long_press_cb(ctx->ctx, ctx->index);
}

static void item_pressed(lv_event_t *e)
{
    ItemCtx *ctx = lv_event_get_user_data(e);
    if(ctx && ctx->on_press) ctx->on_press(ctx->ctx, ctx->index);
}

static void item_released(lv_event_t *e)
{
    ItemCtx *ctx = lv_event_get_user_data(e);
    if(ctx && ctx->on_release) ctx->on_release(ctx->ctx, ctx->index);
}

static lv_obj_t *submenu_get_view(void *module)
{
    return ((ViewSubmenu *)module)->root;
}

static void submenu_reset(void *module)
{
    ViewSubmenu *sm = module;
    lv_obj_clean(sm->list);
    sm->item_count = 0;
    lv_label_set_text(sm->title_lbl, "");
}

static void submenu_destroy(void *module)
{
    ViewSubmenu *sm = module;
    if(sm->root) {
        lv_obj_delete(sm->root);
        sm->root = NULL;
    }
    if(sm->items) {
        free(sm->items);
        sm->items = NULL;
    }
    free(sm);
}

static const ViewModuleVtable submenu_vtable = {
    .get_view = submenu_get_view,
    .reset    = submenu_reset,
    .destroy  = submenu_destroy,
};

ViewSubmenu *view_submenu_alloc(lv_obj_t *parent)
{
    ViewSubmenu *sm = calloc(1, sizeof(ViewSubmenu));

    sm->root = lv_obj_create(parent);
    lv_obj_add_flag(sm->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(sm->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(sm->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(sm->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sm->root, 0, 0);
    lv_obj_set_style_pad_all(sm->root, 0, 0);
    lv_obj_remove_flag(sm->root, LV_OBJ_FLAG_SCROLLABLE);

    sm->header_bar = lv_obj_create(sm->root);
    lv_obj_set_size(sm->header_bar, DISP_W, 80);
    lv_obj_align(sm->header_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(sm->header_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sm->header_bar, 0, 0);
    lv_obj_set_style_pad_all(sm->header_bar, 0, 0);
    lv_obj_remove_flag(sm->header_bar, LV_OBJ_FLAG_SCROLLABLE);

    back_button_create(sm->header_bar);

    sm->title_lbl = lv_label_create(sm->header_bar);
    lv_label_set_text(sm->title_lbl, "");
    lv_obj_set_style_text_font(sm->title_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(sm->title_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(sm->title_lbl, LV_ALIGN_TOP_MID, 0, 28);

    sm->list = lv_obj_create(sm->root);
    lv_obj_set_size(sm->list, DISP_W, DISP_H - 80);
    lv_obj_align(sm->list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_layout(sm->list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sm->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sm->list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(sm->list, 0, 0);
    lv_obj_set_style_pad_top(sm->list, 4, 0);
    lv_obj_set_style_pad_bottom(sm->list, 80, 0);
    lv_obj_set_style_pad_left(sm->list, 0, 0);
    lv_obj_set_style_pad_right(sm->list, 0, 0);
    lv_obj_set_style_bg_opa(sm->list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sm->list, 0, 0);
    lv_obj_set_scrollbar_mode(sm->list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(sm->list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_event_cb(sm->list, scroll_cb, LV_EVENT_SCROLL, NULL);

    return sm;
}

void view_submenu_free(ViewSubmenu *submenu)
{
    submenu_destroy(submenu);
}

ViewModule view_submenu_get_module(ViewSubmenu *submenu)
{
    return (ViewModule){
        .module = submenu,
        .vtable = &submenu_vtable,
    };
}

lv_obj_t *view_submenu_get_view(ViewSubmenu *submenu)
{
    return submenu->root;
}

void view_submenu_reset(ViewSubmenu *submenu)
{
    submenu_reset(submenu);
}

void view_submenu_set_header(ViewSubmenu *submenu, const char *title, lv_color_t accent)
{
    lv_label_set_text(submenu->title_lbl, title);
    lv_obj_set_style_text_color(submenu->title_lbl, accent, 0);
}

static ItemCtx *add_row(ViewSubmenu *submenu, const char *icon, const char *label,
                        lv_color_t color, uint32_t index, void *ctx,
                        lv_obj_t **out_btn)
{
    if(submenu->item_count == submenu->item_cap) {
        uint32_t new_cap = submenu->item_cap ? submenu->item_cap * 2 : 8;
        ItemCtx *grown = realloc(submenu->items, new_cap * sizeof(ItemCtx));
        if(!grown) return NULL;
        submenu->items   = grown;
        submenu->item_cap = new_cap;
    }

    ItemCtx *ictx = &submenu->items[submenu->item_count];
    ictx->cb            = NULL;
    ictx->long_press_cb = NULL;
    ictx->on_press      = NULL;
    ictx->on_release    = NULL;
    ictx->ctx           = ctx;
    ictx->index         = index;

    if(submenu->item_count > 0) {
        lv_obj_t *line = lv_obj_create(submenu->list);
        lv_obj_set_size(line, lv_pct(70), 1);
        lv_obj_set_style_bg_color(line, lv_color_hex(0x444444), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *btn = lv_obj_create(submenu->list);
    lv_obj_set_size(btn, lv_pct(100), MENU_ITEM_HEIGHT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *icn = lv_label_create(btn);
    lv_label_set_text(icn, icon);
    lv_obj_set_style_text_color(icn, color, 0);
    lv_obj_set_style_text_font(icn, FONT_TIME, 0);
    lv_obj_align(icn, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl, FONT_TIME, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 100, 0);

    submenu->item_count++;
    if(out_btn) *out_btn = btn;
    return ictx;
}

void view_submenu_add_item(ViewSubmenu *submenu, const char *icon, const char *label,
                           lv_color_t color, uint32_t index,
                           ViewSubmenuCallback cb, void *ctx)
{
    lv_obj_t *btn = NULL;
    ItemCtx *ictx = add_row(submenu, icon, label, color, index, ctx, &btn);
    if(!ictx) return;
    ictx->cb = cb;
    lv_obj_add_event_cb(btn, item_clicked, LV_EVENT_CLICKED, ictx);

    lv_obj_update_layout(submenu->list);
    apply_barrel(submenu->list);
}

void view_submenu_add_item_holdable(ViewSubmenu *submenu, const char *icon, const char *label,
                                    lv_color_t color, uint32_t index,
                                    ViewSubmenuPressCb on_press,
                                    ViewSubmenuReleaseCb on_release,
                                    void *ctx)
{
    lv_obj_t *btn = NULL;
    ItemCtx *ictx = add_row(submenu, icon, label, color, index, ctx, &btn);
    if(!ictx) return;
    ictx->on_press   = on_press;
    ictx->on_release = on_release;
    lv_obj_add_event_cb(btn, item_pressed,  LV_EVENT_PRESSED,    ictx);
    lv_obj_add_event_cb(btn, item_released, LV_EVENT_RELEASED,   ictx);
    lv_obj_add_event_cb(btn, item_released, LV_EVENT_PRESS_LOST, ictx);

    lv_obj_update_layout(submenu->list);
    apply_barrel(submenu->list);
}

void view_submenu_add_item_ex(ViewSubmenu *submenu,
                              const char *icon, const char *label,
                              const char *subtitle,
                              lv_color_t color, uint32_t index,
                              ViewSubmenuCallback tap_cb,
                              ViewSubmenuCallback long_press_cb,
                              void *ctx)
{
    lv_obj_t *btn = NULL;
    ItemCtx *ictx = add_row(submenu, icon, label, color, index, ctx, &btn);
    if(!ictx) return;
    ictx->cb            = tap_cb;
    ictx->long_press_cb = long_press_cb;

    if(subtitle && subtitle[0] && btn) {
        lv_obj_set_height(btn, MENU_ITEM_HEIGHT + 24);
        uint32_t cnt = lv_obj_get_child_count(btn);
        if(cnt >= 2) {
            lv_obj_t *main_lbl = lv_obj_get_child(btn, cnt - 1);
            lv_obj_align(main_lbl, LV_ALIGN_LEFT_MID, 100, -10);
            lv_obj_t *sub = lv_label_create(btn);
            lv_label_set_text(sub, subtitle);
            lv_obj_set_style_text_color(sub, COLOR_SECONDARY, 0);
            lv_obj_set_style_text_font(sub, FONT_BODY, 0);
            lv_obj_align(sub, LV_ALIGN_LEFT_MID, 100, 16);
        }
    }

    if(tap_cb) {
        lv_obj_add_event_cb(btn, item_clicked, LV_EVENT_CLICKED, ictx);
    }
    if(long_press_cb) {
        lv_obj_add_event_cb(btn, item_long_pressed, LV_EVENT_LONG_PRESSED, ictx);
    }

    lv_obj_update_layout(submenu->list);
    apply_barrel(submenu->list);
}

void view_submenu_set_selected_item(ViewSubmenu *submenu, uint32_t index)
{
    if(index < lv_obj_get_child_count(submenu->list)) {
        lv_obj_t *child = lv_obj_get_child(submenu->list, index);
        lv_obj_scroll_to_view(child, LV_ANIM_OFF);
        lv_obj_update_layout(submenu->list);
        apply_barrel(submenu->list);
    }
}
