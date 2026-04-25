#include "styles.h"

lv_style_t style_screen;
lv_style_t style_card;
lv_style_t style_list_btn;
lv_style_t style_submenu_item;
lv_style_t style_submenu_item_pressed;

void styles_init(void)
{
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, COLOR_BG);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, COLOR_PRIMARY);
    lv_style_set_text_font(&style_screen, FONT_BODY);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_CARD_BG);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 20);
    lv_style_set_pad_all(&style_card, 12);
    lv_style_set_border_width(&style_card, 0);

    lv_style_init(&style_list_btn);
    lv_style_set_bg_color(&style_list_btn, COLOR_CARD_BG);
    lv_style_set_bg_opa(&style_list_btn, LV_OPA_COVER);
    lv_style_set_radius(&style_list_btn, 16);
    lv_style_set_pad_hor(&style_list_btn, 16);
    lv_style_set_pad_ver(&style_list_btn, 14);
    lv_style_set_border_width(&style_list_btn, 0);
    lv_style_set_text_color(&style_list_btn, COLOR_PRIMARY);
    lv_style_set_text_font(&style_list_btn, FONT_MEDIUM);
    lv_style_set_width(&style_list_btn, lv_pct(100));

    lv_style_init(&style_submenu_item);
    lv_style_set_bg_color(&style_submenu_item, COLOR_CARD_BG);
    lv_style_set_bg_opa(&style_submenu_item, LV_OPA_COVER);
    lv_style_set_radius(&style_submenu_item, 18);
    lv_style_set_pad_hor(&style_submenu_item, 16);
    lv_style_set_pad_ver(&style_submenu_item, 16);
    lv_style_set_border_width(&style_submenu_item, 0);
    lv_style_set_text_color(&style_submenu_item, COLOR_PRIMARY);
    lv_style_set_text_font(&style_submenu_item, FONT_MENU);

    lv_style_init(&style_submenu_item_pressed);
    lv_style_set_bg_color(&style_submenu_item_pressed, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_submenu_item_pressed, LV_OPA_COVER);
}
