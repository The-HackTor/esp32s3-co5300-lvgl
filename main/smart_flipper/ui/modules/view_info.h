#ifndef VIEW_INFO_H
#define VIEW_INFO_H

#include <lvgl.h>
#include "ui/view_module.h"

#define VIEW_INFO_MAX_BUTTONS 4

typedef void (*ViewInfoButtonCb)(void *context);

typedef struct ViewInfo ViewInfo;

ViewInfo  *view_info_alloc(lv_obj_t *parent);
void       view_info_free(ViewInfo *info);
ViewModule view_info_get_module(ViewInfo *info);
lv_obj_t  *view_info_get_view(ViewInfo *info);
void       view_info_reset(ViewInfo *info);

void view_info_set_header(ViewInfo *info, const char *title, lv_color_t accent);
void view_info_add_field(ViewInfo *info, const char *key, const char *value, lv_color_t color);
void view_info_add_separator(ViewInfo *info);
void view_info_add_text_block(ViewInfo *info, const char *text,
                              const lv_font_t *font, lv_color_t color);
void view_info_add_button(ViewInfo *info, const char *text, lv_color_t color,
                          ViewInfoButtonCb cb, void *ctx);
void view_info_add_button_row(ViewInfo *info,
                              const char *text1, lv_color_t color1,
                              ViewInfoButtonCb cb1, void *ctx1,
                              const char *text2, lv_color_t color2,
                              ViewInfoButtonCb cb2, void *ctx2);

#endif
