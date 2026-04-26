#ifndef IR_AC_BRAND_H
#define IR_AC_BRAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    AC_MODE_AUTO = 0,
    AC_MODE_COOL,
    AC_MODE_DRY,
    AC_MODE_FAN,
    AC_MODE_HEAT,
    AC_MODE_COUNT,
} AcMode;

typedef enum {
    AC_FAN_AUTO = 0,
    AC_FAN_LOW,
    AC_FAN_MED,
    AC_FAN_HIGH,
    AC_FAN_COUNT,
} AcFan;

typedef struct {
    bool    power;
    AcMode  mode;
    uint8_t temp_c;     /* 16..30 */
    AcFan   fan;
    bool    swing;
} AcState;

typedef struct {
    const char *name;
    esp_err_t (*encode)(const AcState *s,
                        uint16_t **out_timings, size_t *out_n,
                        uint32_t *out_freq_hz);
} AcBrand;

const char *ac_mode_label(AcMode m);
const char *ac_fan_label(AcFan f);

extern const AcBrand ac_brand_samsung;

#endif
