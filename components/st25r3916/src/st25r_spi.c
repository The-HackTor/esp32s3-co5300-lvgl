#include "st25r_spi.h"
#include "platform.h"
#include "esp_log.h"

static const char *TAG = "st25r_spi";

static spi_device_handle_t s_dev;
static gpio_num_t          s_cs = -1;

esp_err_t st25r_spi_init(spi_host_device_t host, gpio_num_t cs_gpio,
                         int clock_hz)
{
    s_cs = cs_gpio;

    const gpio_config_t cs_cfg = {
        .pin_bit_mask = 1ULL << cs_gpio,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cs_cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "cs gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }
    gpio_set_level(cs_gpio, 1);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = clock_hz,
        .mode           = 1,
        .spics_io_num   = -1,
        .queue_size     = 4,
        .flags          = 0,
    };
    err = spi_bus_add_device(host, &dev, &s_dev);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "ST25R3916 SPI device added (CS=%d, %d Hz, mode 1)",
             cs_gpio, clock_hz);
    return ESP_OK;
}

void platform_st25r_spi_select(void)
{
    if(s_cs >= 0) gpio_set_level(s_cs, 0);
}

void platform_st25r_spi_deselect(void)
{
    if(s_cs >= 0) gpio_set_level(s_cs, 1);
}

void platform_st25r_spi_transceive(const uint8_t *txBuf, uint8_t *rxBuf,
                                   uint16_t len)
{
    if(!s_dev || len == 0) return;

    spi_transaction_t t = {
        .length    = (size_t)len * 8,
        .tx_buffer = txBuf,
        .rx_buffer = rxBuf,
    };
    esp_err_t err = spi_device_polling_transmit(s_dev, &t);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "transceive failed: %s", esp_err_to_name(err));
    }
}
