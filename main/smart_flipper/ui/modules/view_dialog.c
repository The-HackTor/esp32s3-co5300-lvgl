#include "view_dialog.h"
#include "ui/styles.h"
#include "ui/widgets/back_button.h"
#include <stdlib.h>

struct ViewDialog {
    lv_obj_t *root;
    lv_obj_t *icon_lbl;
    lv_obj_t *header_lbl;
    lv_obj_t *text_lbl;
    lv_obj_t *btn_row;
    lv_obj_t *btn_left;
    lv_obj_t *btn_center;
    lv_obj_t *btn_right;
    ViewDialogCallback cb;
    void    *cb_ctx;
};

static void btn_cb(lv_event_t *e)
{
    ViewDialog *dlg = lv_event_get_user_data(e);
    if(!dlg->cb) return;

    lv_obj_t *btn = lv_event_get_target(e);
    uint32_t result;
    if(btn == dlg->btn_left)        result = VIEW_DIALOG_RESULT_LEFT;
    else if(btn == dlg->btn_center) result = VIEW_DIALOG_RESULT_CENTER;
    else                            result = VIEW_DIALOG_RESULT_RIGHT;

    dlg->cb(dlg->cb_ctx, result);
}

static lv_obj_t *create_dialog_button(ViewDialog *dlg, lv_obj_t *parent)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 130, 56);
    lv_obj_set_style_bg_color(btn, COLOR_CARD_BG, 0);
    lv_obj_set_style_radius(btn, 26, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, dlg);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_font(lbl, FONT_MENU, 0);
    lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
    lv_obj_center(lbl);

    return btn;
}

/* --- ViewModule vtable --- */
static lv_obj_t *dialog_get_view(void *m) { return ((ViewDialog *)m)->root; }

static void dialog_reset(void *m)
{
    ViewDialog *dlg = m;
    lv_label_set_text(dlg->icon_lbl, "");
    lv_label_set_text(dlg->header_lbl, "");
    lv_label_set_text(dlg->text_lbl, "");
    lv_obj_add_flag(dlg->btn_left, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dlg->btn_center, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dlg->btn_right, LV_OBJ_FLAG_HIDDEN);
    dlg->cb = NULL;
    dlg->cb_ctx = NULL;
}

static void dialog_destroy(void *m)
{
    ViewDialog *dlg = m;
    if(dlg->root) { lv_obj_delete(dlg->root); dlg->root = NULL; }
    free(dlg);
}

static const ViewModuleVtable dialog_vtable = {
    .get_view = dialog_get_view,
    .reset    = dialog_reset,
    .destroy  = dialog_destroy,
};

ViewDialog *view_dialog_alloc(lv_obj_t *parent)
{
    ViewDialog *dlg = calloc(1, sizeof(ViewDialog));

    dlg->root = lv_obj_create(parent);
    lv_obj_add_flag(dlg->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(dlg->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(dlg->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(dlg->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dlg->root, 0, 0);
    lv_obj_set_style_pad_all(dlg->root, 0, 0);
    lv_obj_remove_flag(dlg->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button at top left */
    back_button_create(dlg->root);

    /* Icon */
    dlg->icon_lbl = lv_label_create(dlg->root);
    lv_label_set_text(dlg->icon_lbl, "");
    lv_obj_set_style_text_font(dlg->icon_lbl, FONT_TIME, 0);
    lv_obj_align(dlg->icon_lbl, LV_ALIGN_CENTER, 0, -80);

    /* Header */
    dlg->header_lbl = lv_label_create(dlg->root);
    lv_label_set_text(dlg->header_lbl, "");
    lv_obj_set_style_text_font(dlg->header_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(dlg->header_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(dlg->header_lbl, LV_ALIGN_CENTER, 0, -20);

    /* Text */
    dlg->text_lbl = lv_label_create(dlg->root);
    lv_label_set_text(dlg->text_lbl, "");
    lv_obj_set_style_text_font(dlg->text_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(dlg->text_lbl, COLOR_SECONDARY, 0);
    lv_obj_set_style_text_align(dlg->text_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(dlg->text_lbl, 360);
    lv_obj_align(dlg->text_lbl, LV_ALIGN_CENTER, 0, 20);

    /* Button row */
    dlg->btn_row = lv_obj_create(dlg->root);
    lv_obj_set_size(dlg->btn_row, 400, 70);
    lv_obj_align(dlg->btn_row, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_bg_opa(dlg->btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dlg->btn_row, 0, 0);
    lv_obj_set_style_pad_all(dlg->btn_row, 0, 0);
    lv_obj_set_layout(dlg->btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dlg->btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dlg->btn_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(dlg->btn_row, LV_OBJ_FLAG_SCROLLABLE);

    dlg->btn_left   = create_dialog_button(dlg, dlg->btn_row);
    dlg->btn_center = create_dialog_button(dlg, dlg->btn_row);
    dlg->btn_right  = create_dialog_button(dlg, dlg->btn_row);

    return dlg;
}

void view_dialog_free(ViewDialog *dialog) { dialog_destroy(dialog); }
ViewModule view_dialog_get_module(ViewDialog *dialog)
{
    return (ViewModule){ .module = dialog, .vtable = &dialog_vtable };
}
lv_obj_t *view_dialog_get_view(ViewDialog *dialog) { return dialog->root; }
void view_dialog_reset(ViewDialog *dialog) { dialog_reset(dialog); }

static void set_button(lv_obj_t *btn, const char *text, lv_color_t color)
{
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, text);
}

void view_dialog_set_header(ViewDialog *dlg, const char *text, lv_color_t color)
{
    lv_label_set_text(dlg->header_lbl, text);
    lv_obj_set_style_text_color(dlg->header_lbl, color, 0);
}

void view_dialog_set_text(ViewDialog *dlg, const char *text)
{
    lv_label_set_text(dlg->text_lbl, text);
}

void view_dialog_set_icon(ViewDialog *dlg, const char *icon, lv_color_t color)
{
    lv_label_set_text(dlg->icon_lbl, icon);
    lv_obj_set_style_text_color(dlg->icon_lbl, color, 0);
}

void view_dialog_set_left_button(ViewDialog *dlg, const char *text, lv_color_t color)
{
    set_button(dlg->btn_left, text, color);
}

void view_dialog_set_center_button(ViewDialog *dlg, const char *text, lv_color_t color)
{
    set_button(dlg->btn_center, text, color);
}

void view_dialog_set_right_button(ViewDialog *dlg, const char *text, lv_color_t color)
{
    set_button(dlg->btn_right, text, color);
}

void view_dialog_set_callback(ViewDialog *dlg, ViewDialogCallback cb, void *ctx)
{
    dlg->cb = cb;
    dlg->cb_ctx = ctx;
}
