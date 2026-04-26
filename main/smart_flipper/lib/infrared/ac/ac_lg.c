#include "ac_brand.h"

#include <stdlib.h>

#define LG_HDR_MARK     8500
#define LG_HDR_SPACE    4250
#define LG_BIT_MARK     550
#define LG_ONE_SPACE    1600
#define LG_ZERO_SPACE   550
#define LG_GAP          19980
#define LG_FREQ_HZ      38000

#define LG_MIN_TEMP     16
#define LG_MAX_TEMP     30
#define LG_TEMP_ADJUST  15
#define LG_BITS         28

#define LG_OFF_COMMAND  0x88C0051u
#define LG_SIGNATURE    0x88u

/* Layout (LSB->MSB):
 *   bits 0..3   Sum (4-bit checksum)
 *   bits 4..7   Fan
 *   bits 8..11  Temp (degrees - 15)
 *   bits 12..14 Mode
 *   bits 15..17 reserved (zero)
 *   bits 18..19 Power (00=on, 11=off)
 *   bits 20..27 Sign (0x88)
 */

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_COOL: return 0;
    case AC_MODE_DRY:  return 1;
    case AC_MODE_FAN:  return 2;
    case AC_MODE_AUTO: return 3;
    case AC_MODE_HEAT: return 4;
    default:           return 3;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return 5;
    case AC_FAN_LOW:  return 1;
    case AC_FAN_MED:  return 2;
    case AC_FAN_HIGH: return 4;
    default:          return 5;
    }
}

static uint8_t lg_checksum(uint32_t state)
{
    /* Sum the 4 nibbles above the checksum (Fan+Temp+Mode/res+res/Power). */
    uint32_t v = state >> 4;
    uint8_t sum = 0;
    for(int i = 0; i < 4; i++, v >>= 4) sum += (uint8_t)(v & 0xF);
    return sum & 0x0F;
}

static esp_err_t lg_encode(const AcState *st,
                           uint16_t **out_t, size_t *out_n,
                           uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t temp = st->temp_c;
    if(temp < LG_MIN_TEMP) temp = LG_MIN_TEMP;
    if(temp > LG_MAX_TEMP) temp = LG_MAX_TEMP;

    uint32_t value;
    if(!st->power) {
        value = LG_OFF_COMMAND;
    } else {
        value = ((uint32_t)LG_SIGNATURE         << 20)
              | ((uint32_t)0u                   << 18)   /* Power on = 0b00 */
              | ((uint32_t)(map_mode(st->mode) & 0x07) << 12)
              | ((uint32_t)(temp - LG_TEMP_ADJUST) << 8)
              | ((uint32_t)map_fan(st->fan)     << 4);
        value |= lg_checksum(value);
    }

    /* 28 bits MSB-first, header + each bit (mark+space) + footer mark + gap. */
    const size_t cap = 2 + LG_BITS * 2 + 2;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = LG_HDR_MARK;
    buf[n++] = LG_HDR_SPACE;
    for(int i = LG_BITS - 1; i >= 0; i--) {
        buf[n++] = LG_BIT_MARK;
        buf[n++] = ((value >> i) & 1) ? LG_ONE_SPACE : LG_ZERO_SPACE;
    }
    buf[n++] = LG_BIT_MARK;
    buf[n++] = LG_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = LG_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_lg = {
    .name   = "LG AC",
    .encode = lg_encode,
};
