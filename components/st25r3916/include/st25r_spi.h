#ifndef ST25R_SPI_H
#define ST25R_SPI_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t st25r_spi_init(spi_host_device_t host, gpio_num_t cs_gpio,
                         int clock_hz);

#endif
