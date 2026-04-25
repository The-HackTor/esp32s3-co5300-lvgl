#ifndef VIEW_POPUP_H
#define VIEW_POPUP_H

#include <lvgl.h>
#include "ui/view_module.h"

typedef void (*ViewPopupTimeoutCb)(void *context);

typedef struct ViewPopup ViewPopup;

ViewPopup *view_popup_alloc(lv_obj_t *parent);
void       view_popup_free(ViewPopup *popup);
ViewModule view_popup_get_module(ViewPopup *popup);
lv_obj_t  *view_popup_get_view(ViewPopup *popup);
void       view_popup_reset(ViewPopup *popup);

void view_popup_set_icon(ViewPopup *popup, const char *icon, lv_color_t color);
void view_popup_set_header(ViewPopup *popup, const char *text, lv_color_t color);
void view_popup_set_text(ViewPopup *popup, const char *text);
void view_popup_set_timeout(ViewPopup *popup, uint32_t ms,
                            ViewPopupTimeoutCb cb, void *ctx);

#endif
