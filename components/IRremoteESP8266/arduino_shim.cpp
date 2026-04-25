/*
 * Arduino API shim for IRremoteESP8266 on ESP-IDF.
 * See Arduino.h in this directory for the rationale.
 */

#include "Arduino.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

extern "C" {

void delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void delayMicroseconds(uint32_t us)
{
    esp_rom_delay_us(us);
}

uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}

uint32_t micros(void)
{
    return (uint32_t)esp_timer_get_time();
}

void yield(void)
{
    taskYIELD();
}

/* GPIO stubs: hw_ir.c drives the IR LED through RMT, never through the
 * library's bit-banged sendXxx() pin-toggling path. If a codec function
 * ever tries to actually emit via digitalWrite, it is a wiring bug, not
 * something we silently absorb -- but for compile-time linkage these
 * no-ops are sufficient. */
void pinMode(uint8_t pin, uint8_t mode)              { (void)pin; (void)mode; }
void digitalWrite(uint8_t pin, uint8_t val)          { (void)pin; (void)val; }
int  digitalRead(uint8_t pin)                        { (void)pin; return 0; }
void attachInterrupt(uint8_t pin, void(*isr)(void),
                     int mode)                       { (void)pin; (void)isr; (void)mode; }
void detachInterrupt(uint8_t pin)                    { (void)pin; }

}  /* extern "C" */
