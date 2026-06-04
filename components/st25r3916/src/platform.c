#include "platform.h"
#include "esp_log.h"

static const char *TAG = "st25r_platform";

static SemaphoreHandle_t s_comm_mutex;

static void ensure_mutex(void)
{
    if(s_comm_mutex == NULL) {
        s_comm_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

uint8_t globalCommProtectCnt;

void platform_st25r_protect_comm(void)
{
    ensure_mutex();
    if(s_comm_mutex) {
        xSemaphoreTakeRecursive(s_comm_mutex, portMAX_DELAY);
    }
}

void platform_st25r_unprotect_comm(void)
{
    if(s_comm_mutex) {
        xSemaphoreGiveRecursive(s_comm_mutex);
    }
}

void platform_st25r_global_error(const char *file, long line)
{
    ESP_LOGE(TAG, "RFAL error at %s:%ld", file, line);
}
