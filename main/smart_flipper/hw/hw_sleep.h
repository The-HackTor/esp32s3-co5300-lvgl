#ifndef HW_SLEEP_H
#define HW_SLEEP_H

#include <stdbool.h>
#include <stdint.h>
#include <lvgl.h>

void hw_sleep_init(lv_display_t *disp);
void hw_sleep_set_threshold(uint32_t threshold_ms);
void hw_sleep_inhibit(bool inhibit);

#endif
