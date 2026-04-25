#ifndef VIEW_DIALOG_H
#define VIEW_DIALOG_H

#include <lvgl.h>
#include "ui/view_module.h"

#define VIEW_DIALOG_RESULT_LEFT   0
#define VIEW_DIALOG_RESULT_CENTER 1
#define VIEW_DIALOG_RESULT_RIGHT  2

typedef void (*ViewDialogCallback)(void *context, uint32_t result);

typedef struct ViewDialog ViewDialog;

ViewDialog *view_dialog_alloc(lv_obj_t *parent);
void        view_dialog_free(ViewDialog *dialog);
ViewModule  view_dialog_get_module(ViewDialog *dialog);
lv_obj_t   *view_dialog_get_view(ViewDialog *dialog);
void        view_dialog_reset(ViewDialog *dialog);

void view_dialog_set_header(ViewDialog *dialog, const char *text, lv_color_t color);
void view_dialog_set_text(ViewDialog *dialog, const char *text);
void view_dialog_set_icon(ViewDialog *dialog, const char *icon, lv_color_t color);
void view_dialog_set_left_button(ViewDialog *dialog, const char *text, lv_color_t color);
void view_dialog_set_center_button(ViewDialog *dialog, const char *text, lv_color_t color);
void view_dialog_set_right_button(ViewDialog *dialog, const char *text, lv_color_t color);
void view_dialog_set_callback(ViewDialog *dialog, ViewDialogCallback cb, void *ctx);

#endif
