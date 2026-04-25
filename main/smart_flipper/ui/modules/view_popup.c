#include "view_popup.h"
#include "ui/styles.h"
#include <stdlib.h>

struct ViewPopup {
    lv_obj_t *root;
    lv_obj_t *icon_lbl;
    lv_obj_t *header_lbl;
    lv_obj_t *text_lbl;
    lv_timer_t       *timer;
    ViewPopupTimeoutCb timeout_cb;
    void              *timeout_ctx;
};

static void timer_cb(lv_timer_t *t)
{
    ViewPopup *popup = lv_timer_get_user_data(t);
    lv_timer_delete(popup->timer);
    popup->timer = NULL;
    if(popup->timeout_cb) popup->timeout_cb(popup->timeout_ctx);
}

/* --- ViewModule vtable --- */
static lv_obj_t *popup_get_view(void *m) { return ((ViewPopup *)m)->root; }

static void popup_reset(void *m)
{
    ViewPopup *p = m;
    if(p->timer) { lv_timer_delete(p->timer); p->timer = NULL; }
    lv_label_set_text(p->icon_lbl, "");
    lv_label_set_text(p->header_lbl, "");
    lv_label_set_text(p->text_lbl, "");
    p->timeout_cb = NULL;
    p->timeout_ctx = NULL;
}

static void popup_destroy(void *m)
{
    ViewPopup *p = m;
    if(p->timer) { lv_timer_delete(p->timer); p->timer = NULL; }
    if(p->root) { lv_obj_delete(p->root); p->root = NULL; }
    free(p);
}

static const ViewModuleVtable popup_vtable = {
    .get_view = popup_get_view,
    .reset    = popup_reset,
    .destroy  = popup_destroy,
};

ViewPopup *view_popup_alloc(lv_obj_t *parent)
{
    ViewPopup *p = calloc(1, sizeof(ViewPopup));

    p->root = lv_obj_create(parent);
    lv_obj_add_flag(p->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(p->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(p->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(p->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(p->root, 0, 0);
    lv_obj_set_style_pad_all(p->root, 0, 0);
    lv_obj_remove_flag(p->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon */
    p->icon_lbl = lv_label_create(p->root);
    lv_label_set_text(p->icon_lbl, "");
    lv_obj_set_style_text_font(p->icon_lbl, FONT_TIME, 0);
    lv_obj_align(p->icon_lbl, LV_ALIGN_CENTER, 0, -60);

    /* Header */
    p->header_lbl = lv_label_create(p->root);
    lv_label_set_text(p->header_lbl, "");
    lv_obj_set_style_text_font(p->header_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(p->header_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(p->header_lbl, LV_ALIGN_CENTER, 0, 10);

    /* Text */
    p->text_lbl = lv_label_create(p->root);
    lv_label_set_text(p->text_lbl, "");
    lv_obj_set_style_text_font(p->text_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(p->text_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_align(p->text_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(p->text_lbl, 360);
    lv_obj_align(p->text_lbl, LV_ALIGN_CENTER, 0, 50);

    return p;
}

void view_popup_free(ViewPopup *popup) { popup_destroy(popup); }
ViewModule view_popup_get_module(ViewPopup *popup)
{
    return (ViewModule){ .module = popup, .vtable = &popup_vtable };
}
lv_obj_t *view_popup_get_view(ViewPopup *popup) { return popup->root; }
void view_popup_reset(ViewPopup *popup) { popup_reset(popup); }

void view_popup_set_icon(ViewPopup *p, const char *icon, lv_color_t color)
{
    lv_label_set_text(p->icon_lbl, icon);
    lv_obj_set_style_text_color(p->icon_lbl, color, 0);
}

void view_popup_set_header(ViewPopup *p, const char *text, lv_color_t color)
{
    lv_label_set_text(p->header_lbl, text);
    lv_obj_set_style_text_color(p->header_lbl, color, 0);
}

void view_popup_set_text(ViewPopup *p, const char *text)
{
    lv_label_set_text(p->text_lbl, text);
}

void view_popup_set_timeout(ViewPopup *p, uint32_t ms,
                            ViewPopupTimeoutCb cb, void *ctx)
{
    if(p->timer) { lv_timer_delete(p->timer); p->timer = NULL; }
    p->timeout_cb = cb;
    p->timeout_ctx = ctx;
    if(ms > 0) {
        p->timer = lv_timer_create(timer_cb, ms, p);
        lv_timer_set_repeat_count(p->timer, 1);
    }
}
