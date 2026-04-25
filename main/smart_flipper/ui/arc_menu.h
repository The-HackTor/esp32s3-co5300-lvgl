#ifndef ARC_MENU_H
#define ARC_MENU_H

#include <lvgl.h>
#include <stdbool.h>

void arc_menu_init(void);
void arc_menu_show(void);
void arc_menu_hide(void);
bool arc_menu_is_visible(void);

#endif
