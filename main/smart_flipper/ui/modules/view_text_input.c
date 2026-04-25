#include "view_text_input.h"
#include "ui/styles.h"
#include "ui/widgets/back_button.h"
#include <stdlib.h>
#include <string.h>

struct ViewTextInput {
    lv_obj_t *root;
    lv_obj_t *header_lbl;
    lv_obj_t *textarea;
    lv_obj_t *keyboard;
    char     *buffer;
    size_t    buffer_len;
    ViewTextInputCallback cb;
    void                 *cb_ctx;
};

static void textarea_event_cb(lv_event_t *e)
{
    ViewTextInput *vti = lv_event_get_user_data(e);
    if(!vti->buffer || vti->buffer_len == 0) return;
    const char *text = lv_textarea_get_text(vti->textarea);
    strncpy(vti->buffer, text, vti->buffer_len - 1);
    vti->buffer[vti->buffer_len - 1] = '\0';
}

static void keyboard_event_cb(lv_event_t *e)
{
    ViewTextInput *vti = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_READY) return;
    if(!vti->cb) return;
    const char *text = lv_textarea_get_text(vti->textarea);
    vti->cb(vti->cb_ctx, text);
}

static lv_obj_t *ti_get_view(void *m) { return ((ViewTextInput *)m)->root; }

static void ti_reset(void *m)
{
    ViewTextInput *vti = m;
    lv_label_set_text(vti->header_lbl, "");
    lv_textarea_set_text(vti->textarea, "");
    vti->buffer = NULL;
    vti->buffer_len = 0;
    vti->cb = NULL;
    vti->cb_ctx = NULL;
}

static void ti_destroy(void *m)
{
    ViewTextInput *vti = m;
    if(vti->root) { lv_obj_delete(vti->root); vti->root = NULL; }
    free(vti);
}

static const ViewModuleVtable ti_vtable = {
    .get_view = ti_get_view,
    .reset    = ti_reset,
    .destroy  = ti_destroy,
};

ViewTextInput *view_text_input_alloc(lv_obj_t *parent)
{
    ViewTextInput *vti = calloc(1, sizeof(ViewTextInput));

    vti->root = lv_obj_create(parent);
    lv_obj_add_flag(vti->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(vti->root, DISP_W, DISP_H);
    lv_obj_set_style_bg_color(vti->root, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(vti->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vti->root, 0, 0);
    lv_obj_set_style_pad_all(vti->root, 0, 0);
    lv_obj_remove_flag(vti->root, LV_OBJ_FLAG_SCROLLABLE);

    back_button_create(vti->root);

    vti->header_lbl = lv_label_create(vti->root);
    lv_label_set_text(vti->header_lbl, "");
    lv_obj_set_style_text_font(vti->header_lbl, FONT_TITLE, 0);
    lv_obj_set_style_text_color(vti->header_lbl, COLOR_PRIMARY, 0);
    lv_obj_align(vti->header_lbl, LV_ALIGN_TOP_MID, 0, 24);

    vti->textarea = lv_textarea_create(vti->root);
    lv_textarea_set_one_line(vti->textarea, true);
    lv_textarea_set_placeholder_text(vti->textarea, "");
    lv_obj_set_size(vti->textarea, 380, 56);
    lv_obj_align(vti->textarea, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(vti->textarea, COLOR_CARD_BG, 0);
    lv_obj_set_style_text_color(vti->textarea, COLOR_PRIMARY, 0);
    lv_obj_set_style_text_font(vti->textarea, FONT_MENU, 0);
    lv_obj_set_style_border_color(vti->textarea, COLOR_DIM, 0);
    lv_obj_set_style_border_width(vti->textarea, 1, 0);
    lv_obj_set_style_radius(vti->textarea, 12, 0);
    lv_obj_add_event_cb(vti->textarea, textarea_event_cb, LV_EVENT_VALUE_CHANGED, vti);

    vti->keyboard = lv_keyboard_create(vti->root);
    lv_obj_set_size(vti->keyboard, DISP_W, 280);
    lv_obj_align(vti->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(vti->keyboard, vti->textarea);
    lv_keyboard_set_mode(vti->keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_style_text_font(vti->keyboard, FONT_MEDIUM, 0);
    lv_obj_add_event_cb(vti->keyboard, keyboard_event_cb, LV_EVENT_READY, vti);

    return vti;
}

void view_text_input_free(ViewTextInput *vti) { ti_destroy(vti); }
ViewModule view_text_input_get_module(ViewTextInput *vti)
{
    return (ViewModule){ .module = vti, .vtable = &ti_vtable };
}
lv_obj_t *view_text_input_get_view(ViewTextInput *vti) { return vti->root; }
void view_text_input_reset(ViewTextInput *vti) { ti_reset(vti); }

void view_text_input_set_header(ViewTextInput *vti, const char *title, lv_color_t accent)
{
    lv_label_set_text(vti->header_lbl, title);
    lv_obj_set_style_text_color(vti->header_lbl, accent, 0);
}

void view_text_input_set_buffer(ViewTextInput *vti, char *buffer, size_t buffer_len)
{
    vti->buffer = buffer;
    vti->buffer_len = buffer_len;
    if(buffer && buffer_len > 0) {
        lv_textarea_set_max_length(vti->textarea, buffer_len - 1);
        lv_textarea_set_text(vti->textarea, buffer);
    }
}

void view_text_input_set_callback(ViewTextInput *vti, ViewTextInputCallback cb, void *ctx)
{
    vti->cb = cb;
    vti->cb_ctx = ctx;
}
