#include "hw_imu.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "touch_bsp.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "hw_imu";

#define QMI8658_ADDR        0x6A         /* SDO/SA0 tied LOW per schematic */
#define I2C_TIMEOUT_MS      100

#define REG_WHO_AM_I        0x00
#define REG_CTRL1           0x02
#define REG_CTRL2           0x03         /* accel ODR + scale */
#define REG_CTRL5           0x06
#define REG_CTRL7           0x08         /* enable accel/gyro */
#define REG_AX_L            0x35

#define WHO_AM_I_EXPECTED   0x05

/* CTRL2: aST=0, aFS=001 (4 g full-scale), aODR=0101 (62.5 Hz LP).
 * Low ODR keeps the IMU below ~0.5 mA so it's not a battery hog when
 * the device is idle but the accel-magnitude shake heuristic still
 * has plenty of time resolution (~16 ms per sample). */
#define CTRL2_VAL           ((0u << 7) | (0x1 << 4) | 0x05)

/* CTRL7: aEN=1 (accel on), gEN=0 (gyro off, halves quiescent current). */
#define CTRL7_VAL           0x01

static i2c_master_dev_handle_t s_dev;
static bool                    s_inited;

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, I2C_TIMEOUT_MS);
}

static esp_err_t reg_write_byte(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    return i2c_master_transmit(s_dev, tx, 2, I2C_TIMEOUT_MS);
}

esp_err_t hw_imu_init(void)
{
    if(s_inited) return ESP_OK;

    i2c_master_bus_handle_t bus = touch_bsp_get_bus_handle();
    if(!bus) return ESP_ERR_INVALID_STATE;

    const i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = QMI8658_ADDR,
        .scl_speed_hz    = 400 * 1000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_dev);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "add_device: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t who = 0;
    err = reg_read(REG_WHO_AM_I, &who, 1);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "WHO_AM_I read: %s", esp_err_to_name(err));
        return err;
    }
    if(who != WHO_AM_I_EXPECTED) {
        ESP_LOGW(TAG, "WHO_AM_I=0x%02X, expected 0x%02X", who, WHO_AM_I_EXPECTED);
        /* Some board revs ship a different IMU on the same address.
         * Don't fail init -- accel reads will return zeros and the
         * shake heuristic stays disabled. */
    }

    /* CTRL1 default (auto-increment, INT1 push-pull active-low) is
     * fine; we just configure the accel and turn it on. */
    reg_write_byte(REG_CTRL2, CTRL2_VAL);
    reg_write_byte(REG_CTRL5, 0x00);            /* no LPF on raw output */
    reg_write_byte(REG_CTRL7, CTRL7_VAL);

    s_inited = true;
    ESP_LOGI(TAG, "init: QMI8658 @ 0x%02X, accel 4g/62.5Hz", QMI8658_ADDR);
    return ESP_OK;
}

esp_err_t hw_imu_read_accel_mg(int16_t *ax, int16_t *ay, int16_t *az)
{
    if(!s_inited) return ESP_ERR_INVALID_STATE;
    if(!ax || !ay || !az) return ESP_ERR_INVALID_ARG;

    uint8_t raw[6];
    esp_err_t err = reg_read(REG_AX_L, raw, sizeof(raw));
    if(err != ESP_OK) return err;

    /* 16-bit two's complement, little-endian. Full scale +/- 4g
     * over +/- 32768 LSB -> 1 LSB ~= 122 ug. Convert to mg. */
    int16_t rax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ray = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t raz = (int16_t)((raw[5] << 8) | raw[4]);
    *ax = (int16_t)(((int32_t)rax * 4000) / 32768);
    *ay = (int16_t)(((int32_t)ray * 4000) / 32768);
    *az = (int16_t)(((int32_t)raz * 4000) / 32768);
    return ESP_OK;
}

uint16_t hw_imu_read_magnitude_mg(void)
{
    int16_t ax = 0, ay = 0, az = 0;
    if(hw_imu_read_accel_mg(&ax, &ay, &az) != ESP_OK) return 0;
    int32_t mag2 = (int32_t)ax * ax + (int32_t)ay * ay + (int32_t)az * az;
    /* Integer sqrt -- avoids dragging in libm for one cheap reading. */
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
