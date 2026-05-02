#ifndef SMART_FLIPPER_H
#define SMART_FLIPPER_H

#include <stdint.h>

/* Caller must hold the LVGL mutex and have called lv_init(). */
void smart_flipper_start(void);

/* User brightness preference [0x10..0xFF]. Becomes the FULL-state target;
 * idle dim/blank tiers still ramp below this. */
void app_brightness_set(uint8_t level);

/* Force DBV=0 + DISPOFF. Call before entering any sleep mode so the OLED
 * doesn't retain the last frame across the sleep. */
void app_panel_blank(void);

/* Snap to the user's full brightness + DISPON. Use on sleep wake so the
 * screen returns at full immediately rather than ramping from 0 through
 * idle_dim_tick. */
void app_panel_restore_full(void);

#endif
