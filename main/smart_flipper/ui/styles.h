#ifndef STYLES_H
#define STYLES_H

#include <lvgl.h>

#define DISP_W      466
#define DISP_H      466
#define DISP_RADIUS (DISP_W / 2)

#define COLOR_BG        lv_color_black()
#define COLOR_PRIMARY   lv_color_white()
#define COLOR_SECONDARY lv_color_hex(0x888888)
#define COLOR_DIM       lv_color_hex(0x444444)
#define COLOR_CARD_BG   lv_color_hex(0x1A1A1A)
#define COLOR_RING_BG   lv_color_hex(0x222222)
#define COLOR_CYAN      lv_color_hex(0x00D4AA)
#define COLOR_ORANGE    lv_color_hex(0xFF6B35)
#define COLOR_MAGENTA   lv_color_hex(0xE040FB)
#define COLOR_BLUE      lv_color_hex(0x448AFF)
#define COLOR_RED       lv_color_hex(0xFF5252)
#define COLOR_GREEN     lv_color_hex(0x69F0AE)
#define COLOR_YELLOW    lv_color_hex(0xFFD740)

#define FONT_TIME   &lv_font_montserrat_48
#define FONT_LARGE  &lv_font_montserrat_46
#define FONT_TITLE  &lv_font_montserrat_24
#define FONT_MENU   &lv_font_montserrat_24
#define FONT_MEDIUM &lv_font_montserrat_20
#define FONT_BODY   &lv_font_montserrat_18
#define FONT_SMALL  &lv_font_montserrat_16
#define FONT_TINY   &lv_font_montserrat_14
#define FONT_MONO   &lv_font_montserrat_12

/* Layout constants for circular display */
#define MENU_ITEM_HEIGHT    72
#define MENU_ITEM_SPACING   10
#define TOUCH_TARGET_MIN    52

extern lv_style_t style_screen;
extern lv_style_t style_card;
extern lv_style_t style_list_btn;
extern lv_style_t style_submenu_item;
extern lv_style_t style_submenu_item_pressed;

void styles_init(void);

#endif
