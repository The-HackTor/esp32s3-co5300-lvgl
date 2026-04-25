#ifndef SMART_FLIPPER_H
#define SMART_FLIPPER_H

/* Entry point for the smart_flipper UI on the ESP-IDF port.
 * Must be called while holding the LVGL mutex and after lv_init(). */
void smart_flipper_start(void);

#endif
