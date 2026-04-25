#ifndef HW_RGB_H
#define HW_RGB_H

#include <stdint.h>

/*
 * RGB activity LED driver.
 *
 * Pin map (locked from board schematic):
 *   RED   = GPIO17
 *   GREEN = GPIO7
 *   BLUE  = GPIO0   (BOOT0 strapping pin -- safe because external wiring is
 *                    anode-to-VCC and nothing drives the pin LOW during reset)
 *
 * LEDs are common-anode with R7=R8=R9=0R (no current-limit resistors), so
 * direct GPIO drive would source ~30-40 mA continuously. All channels run
 * through LEDC PWM with output_invert=true and a 15% software duty cap so
 * user code uses 0=OFF / 255=ON semantics without seeing the active-low
 * wiring or risking thermal stress.
 *
 * Module is independent of the IR app -- any future module (SubGHz, NFC)
 * may also call hw_rgb_pulse().
 */

void hw_rgb_init(void);

/* 0..255 per channel; values are scaled into the 15% duty cap internally. */
void hw_rgb_set(uint8_t r, uint8_t g, uint8_t b);
void hw_rgb_off(void);

/* Solid color for duration_ms then automatic off. Replaces any in-flight
 * pulse. Implemented via a single esp_timer one-shot. */
void hw_rgb_pulse(uint8_t r, uint8_t g, uint8_t b, uint32_t duration_ms);

#endif
