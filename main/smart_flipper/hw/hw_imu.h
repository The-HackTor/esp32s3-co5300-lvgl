#ifndef HW_IMU_H
#define HW_IMU_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t hw_imu_init(void);

esp_err_t hw_imu_arm_motion_wake(void);

esp_err_t hw_imu_read_accel_mg(int16_t *ax, int16_t *ay, int16_t *az);

uint16_t hw_imu_read_magnitude_mg(void);

#endif
