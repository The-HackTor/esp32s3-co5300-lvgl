#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#include <stdint.h>
#include "driver/i2c_master.h"

void Touch_Init(void);
uint8_t getTouch(uint16_t *x, uint16_t *y);

/* Expose the I2C0 master bus handle so other on-bus peripherals
 * (PCF85063 RTC, QMI8658 IMU) can register without re-initialising
 * the bus. Returns NULL until Touch_Init() has run. */
i2c_master_bus_handle_t touch_bsp_get_bus_handle(void);

#endif
