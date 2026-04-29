#ifndef HW_IMU_H
#define HW_IMU_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * QPS QMI8658C 6-DoF IMU driver.
 *
 * Hardware:
 *   I2C0 at 0x6A (SDO/SA0 tied LOW per schematic) -- shared bus with
 *     FT3168 touch (0x38) and PCF85063 RTC (0x51).
 *   IMU_INT  -> IO8 (active-low, open-drain via internal pull-up; the
 *     driver configures it as a falling-edge GPIO interrupt source so
 *     the sleep policy can wake on data-ready).
 *
 * Scope: this driver provides probe + raw accelerometer reads. Wake-
 * from-sleep on a hardware AnyMotion event needs the QMI8658's CTRL9
 * host-command interface (cmd 0x13, AnyMotion config table) to be
 * driven; that's not in this commit. The sleep policy uses touch + IR
 * + RTC alarm as wake sources for now and reads the accel from this
 * driver in software for an optional shake-to-wake heuristic.
 */

esp_err_t hw_imu_init(void);

/* Raw accelerometer in milli-g. Returns ESP_OK and populates ax/ay/az
 * on success; values are signed -- e.g. flat-on-back gives az ~= +1000. */
esp_err_t hw_imu_read_accel_mg(int16_t *ax, int16_t *ay, int16_t *az);

/* Magnitude estimate, sqrt(ax*ax+ay*ay+az*az), in milli-g. Cheap proxy
 * for "is the device moving"; compare consecutive readings with a
 * threshold (~150 mg works for shake-to-wake without false-firing on
 * normal handling). */
uint16_t hw_imu_read_magnitude_mg(void);

#endif
