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

i2c_master_bus_handle_t touch_bsp_get_bus_handle(void)
{
  return s_bus_handle;
}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
  /* FT3168 reg 0x02 = touch point count; low nibble is the real count, upper
   * nibble is reserved/undefined on some variants. Without INT/RST wired we
   * see spurious bits occasionally, so mask and bound before trusting it. */
  uint8_t data = 0;
  uint8_t buf[4];
  if (I2C_read_buff(0x02, &data, 1) != 0) return 0;
  uint8_t count = data & 0x0f;
  if (count == 0 || count > 5) return 0;

  if (I2C_read_buff(0x03, buf, 4) != 0) return 0;

  /* Event flag in top 2 bits of buf[0]: 0=Press, 1=LiftUp, 2=Contact, 3=None.
   * Polling mode: accept Press and Contact only; LiftUp is inferred from the
   * next poll returning count==0, so treating it as "no touch" is correct. */
  uint8_t event = (buf[0] >> 6) & 0x03;
  if (event != 0 && event != 2) return 0;

  uint16_t raw_x = (((uint16_t)buf[0] & 0x0f) << 8) | (uint16_t)buf[1];
  uint16_t raw_y = (((uint16_t)buf[2] & 0x0f) << 8) | (uint16_t)buf[3];
  if (raw_x >= EXAMPLE_LCD_H_RES || raw_y >= EXAMPLE_LCD_V_RES) return 0;

  *x = raw_x;
  *y = raw_y;
  return 1;
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
