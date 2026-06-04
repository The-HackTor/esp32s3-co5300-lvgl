#ifndef HW_SPI3_H
#define HW_SPI3_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define HW_SPI3_HOST              SPI3_HOST
#define HW_SPI3_PIN_MOSI          GPIO_NUM_39
#define HW_SPI3_PIN_MISO          GPIO_NUM_40
#define HW_SPI3_PIN_SCLK          GPIO_NUM_41
#define HW_SPI3_MAX_TRANSFER_SZ   4096

esp_err_t hw_spi3_init(void);

#endif
