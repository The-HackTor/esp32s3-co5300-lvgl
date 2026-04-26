#include "ac_brand.h"

void ac_push_byte_lsb(uint16_t *buf, size_t cap, size_t *n, uint8_t b,
                      uint16_t bit_mark, uint16_t one_space, uint16_t zero_space)
{
    for(int bit = 0; bit < 8; bit++) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = bit_mark;
        buf[(*n)++] = (b & (1 << bit)) ? one_space : zero_space;
    }
}

uint8_t ac_sum_bytes(const uint8_t *data, size_t n)
{
    uint16_t sum = 0;
    for(size_t i = 0; i < n; i++) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}

const AcBrand *const ac_brand_table[] = {
    &ac_brand_samsung,
    &ac_brand_daikin,
    &ac_brand_mitsubishi,
    &ac_brand_lg,
    &ac_brand_gree,
};
const size_t ac_brand_count = sizeof(ac_brand_table) / sizeof(ac_brand_table[0]);

const char *ac_mode_label(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return "Auto";
    case AC_MODE_COOL: return "Cool";
    case AC_MODE_DRY:  return "Dry";
    case AC_MODE_FAN:  return "Fan";
    case AC_MODE_HEAT: return "Heat";
    default:           return "?";
    }
}

const char *ac_fan_label(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return "Auto";
    case AC_FAN_LOW:  return "Low";
    case AC_FAN_MED:  return "Med";
    case AC_FAN_HIGH: return "High";
    default:          return "?";
    }
}
