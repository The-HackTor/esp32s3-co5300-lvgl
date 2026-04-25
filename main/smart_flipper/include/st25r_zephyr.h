#ifndef ST25R_ZEPHYR_H
#define ST25R_ZEPHYR_H

/* Stub: no NFC chip on the ESP-IDF port, trigger enable/disable no-op. */
static inline void st25r_trigger_disable(void) { }
static inline void st25r_trigger_enable(void)  { }

#endif
