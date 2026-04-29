#ifndef HW_BAT_H
#define HW_BAT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * Battery voltage / state-of-charge driver.
 *
 * Hardware (ESP32-S3-Touch-AMOLED-1.43, BAT block):
 *   VBAT -> R12 200K -> IO4 (ADC1_CH3) -> R17 100K -> GND
 * Divider ratio 1/3, so ADC reads VBAT/3. ADC1 with 12 dB attenuation
 * spans ~150..3100 mV, comfortably covering the 1100..1400 mV window
 * a 1S LiPo presents at the divider tap.
 *
 * Output is filtered (8-sample sliding average) so a single noisy ADC
 * sample doesn't bounce the status-bar glyph or the SoC trend used by
 * the charging detector.
 */

void hw_bat_init(void);

/* Latest filtered battery voltage in millivolts. Returns 0 if init
 * failed or no sample has been taken yet. */
int  hw_bat_read_mv(void);

/* State of charge in percent [0..100], derived from mV via a typical
 * 1S LiPo discharge curve. */
int  hw_bat_read_soc_pct(void);

/* True if VBAT is above the typical full-charge plateau (>= 4.15 V) or
 * is rising at >= 5 mV/s averaged over the last few seconds. Used by
 * the sleep policy to skip aggressive idle tiers while plugged in. */
bool hw_bat_is_charging(void);

#endif
