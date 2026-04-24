#include <stdio.h>
#include <stdlib.h>
#include "touch_bsp.h"
#include "driver/i2c_master.h"

#define TOUCH_HOST                        I2C_NUM_0
#define EXAMPLE_PIN_NUM_TOUCH_SCL         (GPIO_NUM_48)
#define EXAMPLE_PIN_NUM_TOUCH_SDA         (GPIO_NUM_47)
#define EXAMPLE_PIN_NUM_TOUCH_RST         (-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (-1)
#define EXAMPLE_LCD_H_RES                 466
#define EXAMPLE_LCD_V_RES                 466
#define I2C_ADDR_FT3168                   0x38
#define TOUCH_I2C_TIMEOUT_MS              1000

static i2c_master_bus_handle_t s_bus_handle;
static i2c_master_dev_handle_t s_ft3168_handle;

static uint8_t I2C_writr_buff(uint8_t reg, uint8_t *buf, uint8_t len);
static uint8_t I2C_read_buff(uint8_t reg, uint8_t *buf, uint8_t len);

void Touch_Init(void)
{
  const i2c_master_bus_config_t bus_cfg = {
    .i2c_port = TOUCH_HOST,
    .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
    .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_bus_handle));

  const i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = I2C_ADDR_FT3168,
    .scl_speed_hz = 600 * 1000,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &s_ft3168_handle));

  uint8_t data = 0x00;
  I2C_writr_buff(0x00, &data, 1); //Switch to normal mode
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
  uint8_t data = 0;
  uint8_t buf[4];
  I2C_read_buff(0x02, &data, 1);
  if (data) {
    I2C_read_buff(0x03, buf, 4);
    *x = (((uint16_t)buf[0] & 0x0f) << 8) | (uint16_t)buf[1];
    *y = (((uint16_t)buf[2] & 0x0f) << 8) | (uint16_t)buf[3];
    if (*x > EXAMPLE_LCD_H_RES) *x = EXAMPLE_LCD_H_RES;
    if (*y > EXAMPLE_LCD_V_RES) *y = EXAMPLE_LCD_V_RES;
    return 1;
  }
  return 0;
}

static uint8_t I2C_writr_buff(uint8_t reg, uint8_t *buf, uint8_t len)
{
  uint8_t *pbuf = (uint8_t *)malloc(len + 1);
  pbuf[0] = reg;
  for (uint8_t i = 0; i < len; i++) {
    pbuf[i + 1] = buf[i];
  }
  esp_err_t ret = i2c_master_transmit(s_ft3168_handle, pbuf, len + 1, TOUCH_I2C_TIMEOUT_MS);
  free(pbuf);
  return (uint8_t)ret;
}

static uint8_t I2C_read_buff(uint8_t reg, uint8_t *buf, uint8_t len)
{
  esp_err_t ret = i2c_master_transmit_receive(s_ft3168_handle, &reg, 1, buf, len, TOUCH_I2C_TIMEOUT_MS);
  return (uint8_t)ret;
}
