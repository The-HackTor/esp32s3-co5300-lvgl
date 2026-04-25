#ifndef VIEW_ACTION_H
#define VIEW_ACTION_H

#include <lvgl.h>
#include "ui/view_module.h"

typedef void (*ViewActionCancelCb)(void *context);

typedef struct ViewAction ViewAction;

ViewAction *view_action_alloc(lv_obj_t *parent);
void        view_action_free(ViewAction *action);
ViewModule  view_action_get_module(ViewAction *action);
lv_obj_t   *view_action_get_view(ViewAction *action);
void        view_action_reset(ViewAction *action);

void view_action_set_header(ViewAction *action, const char *title, lv_color_t accent);
void view_action_set_header_detail(ViewAction *action, const char *text);
void view_action_set_text(ViewAction *action, const char *text);
void view_action_set_detail(ViewAction *action, const char *text, lv_color_t color);
void view_action_show_spinner(ViewAction *action, bool show, lv_color_t color);
void view_action_show_arc_progress(ViewAction *action, bool show, lv_color_t color);
void view_action_set_progress(ViewAction *action, int32_t value);  /* 0-100 or angle */
void view_action_set_cancel_button(ViewAction *action, const char *text, lv_color_t color,
                                   ViewActionCancelCb cb, void *ctx);

#endif
