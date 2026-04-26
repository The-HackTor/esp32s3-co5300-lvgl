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

/* Push the 8 LSB-first bits of `b` as bit-mark + (one|zero)-space pairs. */
void ac_push_byte_lsb(uint16_t *buf, size_t cap, size_t *n, uint8_t b,
                      uint16_t bit_mark, uint16_t one_space, uint16_t zero_space);

uint8_t ac_sum_bytes(const uint8_t *data, size_t n);

extern const AcBrand ac_brand_samsung;
extern const AcBrand ac_brand_daikin;
extern const AcBrand ac_brand_mitsubishi;
extern const AcBrand ac_brand_lg;
extern const AcBrand ac_brand_gree;

extern const AcBrand *const ac_brand_table[];
extern const size_t        ac_brand_count;

#endif
