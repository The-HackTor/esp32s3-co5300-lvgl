/* Stub: real bridge UART-DMAs to an ESP32-C5 companion. Standalone S3 logs and drops. */

#include "bridge.h"
#include "esp_log.h"

static const char *TAG = "bridge_stub";

int bridge_init(void)
{
    ESP_LOGW(TAG, "STUB: bridge_init (UART bridge not wired on ESP-IDF build)");
    return 0;
}

int bridge_send(uint8_t type, const void *payload, uint16_t len)
{
    ESP_LOGW(TAG, "STUB: bridge_send type=0x%02x len=%u", type, len);
    (void)payload;
    return 0;
}
