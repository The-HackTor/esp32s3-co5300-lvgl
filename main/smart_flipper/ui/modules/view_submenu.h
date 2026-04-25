#ifndef VIEW_SUBMENU_H
#define VIEW_SUBMENU_H

#include <lvgl.h>
#include "ui/view_module.h"

typedef void (*ViewSubmenuCallback)(void *context, uint32_t index);

typedef struct ViewSubmenu ViewSubmenu;

ViewSubmenu *view_submenu_alloc(lv_obj_t *parent);
void         view_submenu_free(ViewSubmenu *submenu);
ViewModule   view_submenu_get_module(ViewSubmenu *submenu);
lv_obj_t    *view_submenu_get_view(ViewSubmenu *submenu);
void         view_submenu_reset(ViewSubmenu *submenu);

void view_submenu_set_header(ViewSubmenu *submenu, const char *title, lv_color_t accent);
void view_submenu_add_item(ViewSubmenu *submenu, const char *icon, const char *label,
                           lv_color_t color, uint32_t index,
                           ViewSubmenuCallback cb, void *ctx);
void view_submenu_set_selected_item(ViewSubmenu *submenu, uint32_t index);

#endif
