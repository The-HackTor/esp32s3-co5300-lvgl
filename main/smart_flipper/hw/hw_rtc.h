#ifndef HW_RTC_H
#define HW_RTC_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

/*
 * NXP PCF85063ATL real-time clock driver.
 *
 * Hardware:
 *   I2C0 at 0x51 (shared bus -- SDA=IO47, SCL=IO48; same bus as
 *     FT3168 touch and QMI8658 IMU).
 *   RTC_INT  -> IO15 (open-drain, asserted LOW on alarm/timer match,
 *     external 10K pull-up via R2 to 3V3).
 *
 * Notes:
 *   - PCF85063ATL has no Vbat backup pin -- absolute time is lost on
 *     power-off. The chip is still useful for runtime time-of-day,
 *     scheduled wake from light/deep sleep, and the timestamp on the
 *     IR history log.
 *   - Time is stored in BCD across registers 0x04..0x0A. The driver
 *     hides the BCD encoding; callers see plain struct tm.
 *   - Day-of-week (struct tm.tm_wday) is preserved if the caller sets
 *     it on tm_set; otherwise it's left at whatever the chip returned.
 */

esp_err_t hw_rtc_init(void);

/* True if the oscillator-stop flag (OS bit in seconds register) is
 * clear, i.e. the RTC has been running uninterrupted since the last
 * hw_rtc_set_time call. False after a cold boot or VDD brownout --
 * the wall-clock value should be considered invalid. */
bool      hw_rtc_is_valid(void);

esp_err_t hw_rtc_get_time(struct tm *out);
esp_err_t hw_rtc_set_time(const struct tm *in);

/* Schedule an alarm `seconds` from now. AIE is enabled and the AF
 * flag in Control_2 is cleared. Calling again before the alarm fires
 * replaces the previous schedule. */
esp_err_t hw_rtc_set_alarm_in(uint32_t seconds);

/* Acknowledge a fired alarm: clears AF, leaves AIE state untouched.
 * Call from the wake-up handler after the RTC_INT pulse arrives. */
esp_err_t hw_rtc_ack_alarm(void);

esp_err_t hw_rtc_disable_alarm(void);

/* Configure RTC_INT (IO15) as a deep-sleep / light-sleep wake source.
 * Call once after init; the chip drives the line LOW when an enabled
 * alarm matches. */
esp_err_t hw_rtc_enable_wake_pin(bool enable);

#endif
