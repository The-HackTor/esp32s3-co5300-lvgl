#ifndef HW_RGB_H
#define HW_RGB_H

#include <stdint.h>

/*
 * Common-anode RGB on GPIO3/2/1 (R/G/B). No series resistors on the
 * board, so LEDC drives output_invert=true with a 15% duty cap to keep
 * average current safe; callers see plain 0=OFF / 255=ON.
 */

void hw_rgb_init(void);

void hw_rgb_set(uint8_t r, uint8_t g, uint8_t b);
void hw_rgb_off(void);

/* Replaces any in-flight pulse. */
void hw_rgb_pulse(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms);

#endif
