#ifndef VIEW_TEXT_INPUT_H
#define VIEW_TEXT_INPUT_H

#include <lvgl.h>
#include <stddef.h>
#include "ui/view_module.h"

typedef void (*ViewTextInputCallback)(void *context, const char *text);

typedef struct ViewTextInput ViewTextInput;

ViewTextInput *view_text_input_alloc(lv_obj_t *parent);
void           view_text_input_free(ViewTextInput *vti);
ViewModule     view_text_input_get_module(ViewTextInput *vti);
lv_obj_t      *view_text_input_get_view(ViewTextInput *vti);
void           view_text_input_reset(ViewTextInput *vti);

void view_text_input_set_header(ViewTextInput *vti, const char *title, lv_color_t accent);
void view_text_input_set_buffer(ViewTextInput *vti, char *buffer, size_t buffer_len);
void view_text_input_set_callback(ViewTextInput *vti, ViewTextInputCallback cb, void *ctx);

#endif
