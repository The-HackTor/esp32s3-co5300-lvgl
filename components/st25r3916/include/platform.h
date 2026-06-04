#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_timer.h"

struct st25r_kern_sem {
    SemaphoreHandle_t handle;
};
#define k_sem  st25r_kern_sem

typedef TickType_t k_timeout_t;
#define K_FOREVER  portMAX_DELAY
#define K_MSEC(ms) ((k_timeout_t)pdMS_TO_TICKS(ms))

static inline int k_sem_take(struct st25r_kern_sem *s, k_timeout_t t)
{
    if(!s || !s->handle) return -1;
    return xSemaphoreTake(s->handle, t) == pdTRUE ? 0 : -1;
}

static inline void k_sem_reset(struct st25r_kern_sem *s)
{
    if(!s || !s->handle) return;
    while(xSemaphoreTake(s->handle, 0) == pdTRUE) {  }
}

static inline int64_t k_uptime_get(void)
{
    return esp_timer_get_time() / 1000;
}

struct st25r_kern_sem *st25r_irq_wait_sem(void);

#include "st_errno.h"

#define ST25R3916

void platform_st25r_protect_comm(void);
void platform_st25r_unprotect_comm(void);
#define platformProtectST25RComm()                platform_st25r_protect_comm()
#define platformUnprotectST25RComm()              platform_st25r_unprotect_comm()
#define platformProtectST25RIrqStatus()           platformProtectST25RComm()
#define platformUnprotectST25RIrqStatus()         platformUnprotectST25RComm()
#define platformProtectWorker()                   taskYIELD()
#define platformUnprotectWorker()

#define platformIrqST25RSetCallback(cb)
#define platformIrqST25RPinInitialize()

#define platformLedsInitialize()
#define platformLedOff(port, pin)
#define platformLedOn(port, pin)
#define platformLedToogle(port, pin)

#define platformGpioSet(port, pin)
#define platformGpioClear(port, pin)
#define platformGpioToogle(port, pin)
#define platformGpioIsHigh(port, pin)             (0)
#define platformGpioIsLow(port, pin)              (0)

extern bool     timerIsExpired(uint32_t timer);
extern uint32_t timerCalculateTimer(uint16_t time);
#define platformTimerCreate(t)                    timerCalculateTimer(t)
#define platformTimerDestroy(timer)
static inline bool platform_timer_yield_expired_(uint32_t timer)
{
    taskYIELD();
    return timerIsExpired(timer);
}
#define platformTimerIsExpired(timer)             platform_timer_yield_expired_(timer)
#define platformDelay(t)                          vTaskDelay(pdMS_TO_TICKS(t))
#define platformGetSysTick()                      ((uint32_t)k_uptime_get())

void platform_st25r_global_error(const char *file, long line);
#define platformErrorHandle()                     platform_st25r_global_error(__FILE__, __LINE__)

void platform_st25r_spi_select(void);
void platform_st25r_spi_deselect(void);
void platform_st25r_spi_transceive(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);
#define platformSpiSelect()                       platform_st25r_spi_select()
#define platformSpiDeselect()                     platform_st25r_spi_deselect()
#define platformSpiTxRx(txBuf, rxBuf, len)        platform_st25r_spi_transceive(txBuf, rxBuf, len)

#define platformLog(...)

extern uint8_t globalCommProtectCnt;

#define RFAL_SUPPORT_MODE_POLL_NFCA            true
#define RFAL_SUPPORT_MODE_POLL_NFCB            false
#define RFAL_SUPPORT_MODE_POLL_NFCF            false
#define RFAL_SUPPORT_MODE_POLL_NFCV            false
#define RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P      false
#define RFAL_SUPPORT_MODE_LISTEN_NFCA          true
#define RFAL_SUPPORT_MODE_LISTEN_NFCB          false
#define RFAL_SUPPORT_MODE_LISTEN_NFCF          false
#define RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P    false

#define RFAL_FEATURE_LISTEN_MODE               true
#define RFAL_FEATURE_WAKEUP_MODE               false
#define RFAL_FEATURE_LOWPOWER_MODE             false
#define RFAL_FEATURE_NFCA                      true
#define RFAL_FEATURE_NFCB                      false
#define RFAL_FEATURE_NFCF                      false
#define RFAL_FEATURE_NFCV                      false
#define RFAL_FEATURE_T1T                       false
#define RFAL_FEATURE_T2T                       false
#define RFAL_FEATURE_T4T                       false
#define RFAL_FEATURE_ST25TB                    false
#define RFAL_FEATURE_ST25xV                    false
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG     true
#define RFAL_FEATURE_DPO                       false
#define RFAL_FEATURE_ISO_DEP                   false
#define RFAL_FEATURE_ISO_DEP_POLL              false
#define RFAL_FEATURE_ISO_DEP_LISTEN            false
#define RFAL_FEATURE_NFC_DEP                   false

#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN    256
#define RFAL_FEATURE_NFC_DEP_BLOCK_MAX_LEN     254
#define RFAL_FEATURE_NFC_RF_BUF_LEN            1024
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN      512
#define RFAL_FEATURE_NFC_DEP_PDU_MAX_LEN       512

#define RFAL_ANALOG_CONFIG_CUSTOM

#endif
