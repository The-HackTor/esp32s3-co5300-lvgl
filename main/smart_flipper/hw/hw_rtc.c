#include "hw_rtc.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "touch_bsp.h"

#include <string.h>
#include <sys/time.h>

static const char *TAG = "hw_rtc";

#define PCF85063_ADDR        0x51
#define PCF85063_INT_GPIO    GPIO_NUM_15
#define I2C_TIMEOUT_MS       100

#define REG_CONTROL_1        0x00
#define REG_CONTROL_2        0x01
#define REG_OFFSET           0x02
#define REG_RAM              0x03
#define REG_SECONDS          0x04
#define REG_MINUTES          0x05
#define REG_HOURS            0x06
#define REG_DAYS             0x07
#define REG_WEEKDAYS         0x08
#define REG_MONTHS           0x09
#define REG_YEARS            0x0A
#define REG_SECOND_ALARM     0x0B
#define REG_MINUTE_ALARM     0x0C
#define REG_HOUR_ALARM       0x0D
#define REG_DAY_ALARM        0x0E
#define REG_WEEKDAY_ALARM    0x0F

#define CTRL2_AF             (1u << 3)
#define CTRL2_AIE            (1u << 7)
#define SEC_OS_FLAG          (1u << 7)         /* oscillator stopped flag */
#define ALARM_DISABLE        (1u << 7)         /* set in alarm reg to disable */

static i2c_master_dev_handle_t s_dev;
static bool                    s_inited;

static uint8_t bcd_encode(uint8_t v) { return ((v / 10) << 4) | (v % 10); }
static uint8_t bcd_decode(uint8_t v) { return ((v >> 4) * 10) + (v & 0x0F); }

static esp_err_t reg_read(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, I2C_TIMEOUT_MS);
}

static esp_err_t reg_write(uint8_t reg, const uint8_t *buf, size_t n)
{
    /* Compose [reg, payload...] into a stack buffer. PCF85063 supports
     * auto-increment so a single transmit covers contiguous writes. */
    uint8_t tx[8];
    if(n + 1 > sizeof(tx)) return ESP_ERR_INVALID_SIZE;
    tx[0] = reg;
    memcpy(&tx[1], buf, n);
    return i2c_master_transmit(s_dev, tx, n + 1, I2C_TIMEOUT_MS);
}

static esp_err_t reg_write_byte(uint8_t reg, uint8_t val)
{
    return reg_write(reg, &val, 1);
}

esp_err_t hw_rtc_init(void)
{
    if(s_inited) return ESP_OK;

    i2c_master_bus_handle_t bus = touch_bsp_get_bus_handle();
    if(!bus) {
        ESP_LOGE(TAG, "I2C bus not initialised; call Touch_Init first");
        return ESP_ERR_INVALID_STATE;
    }

    const i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = PCF85063_ADDR,
        .scl_speed_hz    = 400 * 1000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_dev);
    if(err != ESP_OK) { ESP_LOGE(TAG, "add_device: %s", esp_err_to_name(err)); return err; }

    /* Probe: read Control_1, ensure STOP=0 and capacitor select = 7 pF (default). */
    uint8_t ctrl1 = 0;
    err = reg_read(REG_CONTROL_1, &ctrl1, 1);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "probe: %s", esp_err_to_name(err));
        return err;
    }
    /* CAP_SEL=0 (7 pF), 12_24=0 (24 hour), STOP=0, RESET=0. */
    err = reg_write_byte(REG_CONTROL_1, 0x00);
    if(err != ESP_OK) return err;

    /* Disable all alarms, clear flags. */
    uint8_t alarm_disable[5] = {
        ALARM_DISABLE, ALARM_DISABLE, ALARM_DISABLE,
        ALARM_DISABLE, ALARM_DISABLE,
    };
    reg_write(REG_SECOND_ALARM, alarm_disable, 5);
    reg_write_byte(REG_CONTROL_2, 0x00);

    s_inited = true;
    ESP_LOGI(TAG, "init: PCF85063 @ 0x%02X on shared I2C0", PCF85063_ADDR);

    /* If the RTC has retained a valid time across reset (oscillator
     * stop flag clear), seed the system clock from it so gettimeofday /
     * time(NULL) return wall-clock from boot. PCF85063ATL has no Vbat,
     * so this only fires if we were warm-reset (deep-sleep wake, RST
     * button) -- a fresh power-on always starts at uptime. */
    if(hw_rtc_is_valid()) {
        struct tm now;
        if(hw_rtc_get_time(&now) == ESP_OK) {
            time_t t = mktime(&now);
            if(t != (time_t)-1) {
                struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                ESP_LOGI(TAG, "system clock seeded from RTC: %lld", (long long)t);
            }
        }
    } else {
        ESP_LOGW(TAG, "RTC oscillator-stop flag set; wall-clock invalid until set");
    }

    return ESP_OK;
}

bool hw_rtc_is_valid(void)
{
    if(!s_inited) return false;
    uint8_t sec = 0;
    if(reg_read(REG_SECONDS, &sec, 1) != ESP_OK) return false;
    return (sec & SEC_OS_FLAG) == 0;
}

esp_err_t hw_rtc_get_time(struct tm *out)
{
    if(!s_inited || !out) return ESP_ERR_INVALID_STATE;
    uint8_t buf[7];
    esp_err_t err = reg_read(REG_SECONDS, buf, sizeof(buf));
    if(err != ESP_OK) return err;

    out->tm_sec  = bcd_decode(buf[0] & 0x7F);
    out->tm_min  = bcd_decode(buf[1] & 0x7F);
    out->tm_hour = bcd_decode(buf[2] & 0x3F);
    out->tm_mday = bcd_decode(buf[3] & 0x3F);
    out->tm_wday = buf[4] & 0x07;
    out->tm_mon  = bcd_decode(buf[5] & 0x1F) - 1;     /* tm_mon is 0..11 */
    out->tm_year = bcd_decode(buf[6]) + 100;          /* PCF85063 is 00..99, offset 2000; tm is from 1900 */
    out->tm_isdst = -1;
    return ESP_OK;
}

esp_err_t hw_rtc_set_time(const struct tm *in)
{
    if(!s_inited || !in) return ESP_ERR_INVALID_STATE;

    uint8_t buf[7];
    /* Writing seconds with bit7 clear also clears the OS flag, so this
     * single write both sets the time and marks the value valid. */
    buf[0] = bcd_encode(in->tm_sec  & 0x7F);
    buf[1] = bcd_encode(in->tm_min  & 0x7F);
    buf[2] = bcd_encode(in->tm_hour & 0x3F);
    buf[3] = bcd_encode(in->tm_mday & 0x3F);
    buf[4] = (uint8_t)(in->tm_wday & 0x07);
    buf[5] = bcd_encode((in->tm_mon + 1) & 0x1F);
    int yy = in->tm_year - 100;                       /* tm_year is from 1900; PCF85063 wants 00..99 */
    if(yy < 0) yy = 0;
    if(yy > 99) yy = 99;
    buf[6] = bcd_encode((uint8_t)yy);
    return reg_write(REG_SECONDS, buf, sizeof(buf));
}

esp_err_t hw_rtc_set_alarm_in(uint32_t seconds)
{
    if(!s_inited) return ESP_ERR_INVALID_STATE;
    if(seconds == 0) seconds = 1;

    struct tm now;
    esp_err_t err = hw_rtc_get_time(&now);
    if(err != ESP_OK) return err;

    time_t t = mktime(&now);
    if(t == (time_t)-1) return ESP_FAIL;
    t += (time_t)seconds;
    struct tm fire;
    if(!gmtime_r(&t, &fire)) return ESP_FAIL;

    /* Match second + minute + hour + day; skip weekday. AE=0 enables. */
    uint8_t alarm[5];
    alarm[0] = bcd_encode(fire.tm_sec  & 0x7F);
    alarm[1] = bcd_encode(fire.tm_min  & 0x7F);
    alarm[2] = bcd_encode(fire.tm_hour & 0x3F);
    alarm[3] = bcd_encode(fire.tm_mday & 0x3F);
    alarm[4] = ALARM_DISABLE;                         /* weekday alarm off */
    err = reg_write(REG_SECOND_ALARM, alarm, sizeof(alarm));
    if(err != ESP_OK) return err;

    /* Clear AF, enable AIE so RTC_INT goes LOW on match. */
    return reg_write_byte(REG_CONTROL_2, CTRL2_AIE);
}

esp_err_t hw_rtc_ack_alarm(void)
{
    if(!s_inited) return ESP_ERR_INVALID_STATE;
    /* Read-modify-write to preserve AIE. */
    uint8_t c2 = 0;
    esp_err_t err = reg_read(REG_CONTROL_2, &c2, 1);
    if(err != ESP_OK) return err;
    c2 &= ~CTRL2_AF;
    return reg_write_byte(REG_CONTROL_2, c2);
}

esp_err_t hw_rtc_disable_alarm(void)
{
    if(!s_inited) return ESP_ERR_INVALID_STATE;
    uint8_t alarm_disable[5] = {
        ALARM_DISABLE, ALARM_DISABLE, ALARM_DISABLE,
        ALARM_DISABLE, ALARM_DISABLE,
    };
    reg_write(REG_SECOND_ALARM, alarm_disable, 5);
    return reg_write_byte(REG_CONTROL_2, 0x00);
}

esp_err_t hw_rtc_enable_wake_pin(bool enable)
{
    if(enable) {
        const gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << PCF85063_INT_GPIO,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,    /* external 10K pull-up via R2 */
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_NEGEDGE,
        };
        esp_err_t err = gpio_config(&cfg);
        if(err != ESP_OK) return err;
        return esp_sleep_enable_ext1_wakeup_io(1ULL << PCF85063_INT_GPIO,
                                                ESP_EXT1_WAKEUP_ANY_LOW);
    }
    return esp_sleep_disable_ext1_wakeup_io(1ULL << PCF85063_INT_GPIO);
}
