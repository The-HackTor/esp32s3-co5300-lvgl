#ifndef LIST_MENU_H
#define LIST_MENU_H

#include <lvgl.h>

typedef void (*list_menu_cb_t)(uint32_t index, void *user_data);

lv_obj_t *list_menu_create(lv_obj_t *parent);
void      list_menu_add_item(lv_obj_t *list, const char *icon, const char *text,
                             lv_color_t color, list_menu_cb_t cb, void *user_data);
void      list_menu_clear(lv_obj_t *list);
void      list_menu_refresh(lv_obj_t *list);

#endif
