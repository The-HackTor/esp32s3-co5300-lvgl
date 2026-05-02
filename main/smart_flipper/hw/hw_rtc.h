#ifndef HW_RTC_H
#define HW_RTC_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

/*
 * PCF85063ATL RTC. I2C0 @ 0x51 (shared with touch + IMU).
 * RTC_INT -> IO15, open-drain, ext 10K pull-up to 3V3.
 * No Vbat -- wall-clock is lost on power-off (warm-reset preserved).
 */

esp_err_t hw_rtc_init(void);

/* Oscillator-stop bit clear: time is trustworthy. */
bool      hw_rtc_is_valid(void);

esp_err_t hw_rtc_get_time(struct tm *out);
esp_err_t hw_rtc_set_time(const struct tm *in);

/* Replaces any prior schedule. Enables AIE, clears AF. */
esp_err_t hw_rtc_set_alarm_in(uint32_t seconds);

/* Clear AF, preserve AIE. */
esp_err_t hw_rtc_ack_alarm(void);

esp_err_t hw_rtc_disable_alarm(void);

esp_err_t hw_rtc_enable_wake_pin(bool enable);

#endif
