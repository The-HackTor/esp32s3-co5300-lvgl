#ifndef ST25R_TRIGGER_H
#define ST25R_TRIGGER_H

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t st25r_trigger_init(gpio_num_t irq_gpio);
void      st25r_trigger_enable(void);
void      st25r_trigger_disable(void);

#endif
