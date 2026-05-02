#include "hw_imu.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "touch_bsp.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "hw_imu";

#define I2C_TIMEOUT_MS  100

#define ADDR_LOW        0x6A
#define ADDR_HIGH       0x6B

#define REG_WHO_AM_I    0x00
#define REG_REVISION    0x01
#define REG_CTRL1       0x02
#define REG_CTRL2       0x03
#define REG_CTRL3       0x04
#define REG_CTRL5       0x06
#define REG_CTRL7       0x08
#define REG_CTRL8       0x09
#define REG_CTRL9       0x0A
#define REG_CAL1_L      0x0B
#define REG_CAL1_H      0x0C
#define REG_CAL2_L      0x0D
#define REG_CAL2_H      0x0E
#define REG_CAL3_L      0x0F
#define REG_CAL3_H      0x10
#define REG_CAL4_L      0x11
#define REG_CAL4_H      0x12
#define REG_STATUS_INT  0x2D
#define REG_STATUS0     0x2E
#define REG_STATUS1     0x2F
#define REG_AX_L        0x35

#define WHO_AM_I_VAL    0x05

#define CTRL1_INT1_EN   0x08
#define CTRL1_INT2_EN   0x10
#define CTRL1_BASE      0x60

#define CTRL7_ACC_EN    0x01
#define CTRL7_GYR_EN    0x02
#define CTRL7_ACCGYR_EN (CTRL7_ACC_EN | CTRL7_GYR_EN)

#define CTRL8_VAL       0xC0

#define ACC_RANGE_8G    (0x02 << 4)
#define ACC_ODR_250HZ   0x05
#define GYR_RANGE_1024  (0x06 << 4)
#define GYR_ODR_250HZ   0x05

#define CTRL9_NOP       0x00
#define CTRL9_MOTION    0x0E
#define CTRL9_ON_DEMAND 0xA2

#define ACC_LSB_PER_G   4096    /* 8g range -> 32768/8 LSB/g */

static i2c_master_dev_handle_t s_dev;
static bool                    s_inited;
static uint8_t                 s_addr;

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, I2C_TIMEOUT_MS);
}

static esp_err_t reg_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    return i2c_master_transmit(s_dev, tx, 2, I2C_TIMEOUT_MS);
}

static esp_err_t bind_addr(i2c_master_bus_handle_t bus, uint8_t addr)
{
    if(s_dev) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    const i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = 400 * 1000,
    };
    return i2c_master_bus_add_device(bus, &cfg, &s_dev);
}

static esp_err_t probe_who_am_i(i2c_master_bus_handle_t bus, uint8_t addr)
{
    esp_err_t err = bind_addr(bus, addr);
    if(err != ESP_OK) return err;
    uint8_t who = 0;
    for(int i = 0; i < 5; i++) {
        if(reg_read(REG_WHO_AM_I, &who, 1) == ESP_OK && who == WHO_AM_I_VAL) {
            s_addr = addr;
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t ctrl9_cmd(uint8_t cmd)
{
    esp_err_t err = reg_write(REG_CTRL9, cmd);
    if(err != ESP_OK) return err;

    uint8_t st = 0;
    for(int i = 0; i < 100; i++) {
        if(reg_read(REG_STATUS_INT, &st, 1) == ESP_OK && (st & 0x80)) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    err = reg_write(REG_CTRL9, CTRL9_NOP);
    if(err != ESP_OK) return err;
    for(int i = 0; i < 100; i++) {
        if(reg_read(REG_STATUS_INT, &st, 1) == ESP_OK && !(st & 0x80)) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ESP_OK;
}

static esp_err_t configure_for_accel_gyro(void)
{
    esp_err_t err;
    err = reg_write(REG_CTRL7, 0x00);            if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL2, ACC_RANGE_8G | ACC_ODR_250HZ);   if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL5, 0x00);            if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL3, GYR_RANGE_1024 | GYR_ODR_250HZ); if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL8, CTRL8_VAL);       if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL1, CTRL1_BASE | CTRL1_INT2_EN | CTRL1_INT1_EN);
    if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL7, CTRL7_ACCGYR_EN); if(err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(2));
    return ESP_OK;
}

esp_err_t hw_imu_init(void)
{
    if(s_inited) return ESP_OK;

    i2c_master_bus_handle_t bus = touch_bsp_get_bus_handle();
    if(!bus) return ESP_ERR_INVALID_STATE;

    if(probe_who_am_i(bus, ADDR_HIGH) != ESP_OK &&
       probe_who_am_i(bus, ADDR_LOW)  != ESP_OK) {
        ESP_LOGE(TAG, "QMI8658 not detected at 0x6A or 0x6B");
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t rev = 0;
    reg_read(REG_REVISION, &rev, 1);

    esp_err_t err = ctrl9_cmd(CTRL9_ON_DEMAND);
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "on-demand cali failed: %s", esp_err_to_name(err));
    }

    err = configure_for_accel_gyro();
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "config: %s", esp_err_to_name(err));
        return err;
    }

    s_inited = true;
    ESP_LOGI(TAG, "init: QMI8658 @ 0x%02X rev=0x%02X (8g/250Hz, 1024dps/250Hz)",
             s_addr, rev);

    hw_imu_arm_motion_wake();
    return ESP_OK;
}

esp_err_t hw_imu_arm_motion_wake(void)
{
    if(!s_inited) return ESP_ERR_INVALID_STATE;

    esp_err_t err = reg_write(REG_CTRL7, 0x00);
    if(err != ESP_OK) return err;

    reg_write(REG_CAL1_L, 0x03);
    reg_write(REG_CAL1_H, 0x03);
    reg_write(REG_CAL2_L, 0x03);
    reg_write(REG_CAL2_H, 0x02);
    reg_write(REG_CAL3_L, 0x02);
    reg_write(REG_CAL3_H, 0x02);
    reg_write(REG_CAL4_L, 0xF7);
    reg_write(REG_CAL4_H, 0x01);
    err = ctrl9_cmd(CTRL9_MOTION);
    if(err != ESP_OK) return err;

    reg_write(REG_CAL1_L, 0x03);
    reg_write(REG_CAL1_H, 0x01);
    reg_write(REG_CAL2_L, 0x2C);
    reg_write(REG_CAL2_H, 0x01);
    reg_write(REG_CAL3_L, 0x64);
    reg_write(REG_CAL3_H, 0x00);
    reg_write(REG_CAL4_H, 0x02);
    err = ctrl9_cmd(CTRL9_MOTION);
    if(err != ESP_OK) return err;

    err = reg_write(REG_CTRL8, 0x80);
    if(err != ESP_OK) return err;
    err = reg_write(REG_CTRL7, CTRL7_ACCGYR_EN);
    if(err != ESP_OK) return err;

    uint8_t scratch;
    reg_read(REG_STATUS_INT, &scratch, 1);
    reg_read(REG_STATUS0,    &scratch, 1);
    reg_read(REG_STATUS1,    &scratch, 1);

    ESP_LOGI(TAG, "AnyMotion armed (INT pulses on movement)");
    return ESP_OK;
}

esp_err_t hw_imu_read_accel_mg(int16_t *ax, int16_t *ay, int16_t *az)
{
    if(!s_inited)            return ESP_ERR_INVALID_STATE;
    if(!ax || !ay || !az)    return ESP_ERR_INVALID_ARG;

    uint8_t raw[6];
    esp_err_t err = reg_read(REG_AX_L, raw, sizeof(raw));
    if(err != ESP_OK) return err;

    int16_t rax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ray = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t raz = (int16_t)((raw[5] << 8) | raw[4]);
    *ax = (int16_t)(((int32_t)rax * 1000) / ACC_LSB_PER_G);
    *ay = (int16_t)(((int32_t)ray * 1000) / ACC_LSB_PER_G);
    *az = (int16_t)(((int32_t)raz * 1000) / ACC_LSB_PER_G);
    return ESP_OK;
}

uint16_t hw_imu_read_magnitude_mg(void)
{
    int16_t ax = 0, ay = 0, az = 0;
    if(hw_imu_read_accel_mg(&ax, &ay, &az) != ESP_OK) return 0;
    int32_t mag2 = (int32_t)ax * ax + (int32_t)ay * ay + (int32_t)az * az;
    uint32_t r = 0;
    uint32_t bit = 1u << 30;
    while(bit > (uint32_t)mag2) bit >>= 2;
    uint32_t v = (uint32_t)mag2;
    while(bit) {
        if(v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint16_t)(r > UINT16_MAX ? UINT16_MAX : r);
}
