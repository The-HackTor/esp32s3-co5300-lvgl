#ifndef HW_BAT_H
#define HW_BAT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * VBAT -> 200K -> IO4 (ADC1_CH3) -> 100K -> GND. Divider 1/3, ADC1 @ 12 dB
 * (~150..3100 mV) covers the 1100..1400 mV tap window for a 1S LiPo.
 * Sliding-8 filter on top.
 */

void hw_bat_init(void);

int  hw_bat_read_mv(void);
int  hw_bat_read_soc_pct(void);

/* Plateau (>= 4.15 V) OR rising >= CHARGING_TREND_THRESHOLD over the
 * trend window. Sleep policy uses this to skip aggressive idle tiers. */
bool hw_bat_is_charging(void);

#endif
