#include "hw_spi3.h"
#include "esp_log.h"

static const char *TAG = "hw_spi3";
static bool s_initialized;

esp_err_t hw_spi3_init(void)
{
    if(s_initialized) return ESP_OK;

    const spi_bus_config_t bus = {
        .mosi_io_num     = HW_SPI3_PIN_MOSI,
        .miso_io_num     = HW_SPI3_PIN_MISO,
        .sclk_io_num     = HW_SPI3_PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = HW_SPI3_MAX_TRANSFER_SZ,
    };
    esp_err_t err = spi_bus_initialize(HW_SPI3_HOST, &bus, SPI_DMA_CH_AUTO);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }
    s_initialized = true;
    ESP_LOGI(TAG, "SPI3 bus up (MOSI=%d MISO=%d SCLK=%d)",
             HW_SPI3_PIN_MOSI, HW_SPI3_PIN_MISO, HW_SPI3_PIN_SCLK);
    return ESP_OK;
}
