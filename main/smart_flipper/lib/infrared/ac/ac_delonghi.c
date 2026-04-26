#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define DEL_BYTES        8
#define DEL_FREQ_HZ      38000

#define DEL_HDR_MARK     8984
#define DEL_HDR_SPACE    4200
#define DEL_BIT_MARK     572
#define DEL_ONE_SPACE    1558
#define DEL_ZERO_SPACE   510
/* kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define DEL_GAP          30000

#define DEL_MIN_TEMP     18
#define DEL_MAX_TEMP     32

#define DEL_MODE_COOL    0b000
#define DEL_MODE_DRY     0b001
#define DEL_MODE_FAN     0b010
#define DEL_MODE_AUTO    0b100

#define DEL_FAN_AUTO     0b00
#define DEL_FAN_HIGH     0b01
#define DEL_FAN_MED      0b10
#define DEL_FAN_LOW      0b11

#define DEL_CHECKSUM_BIT 56

/* Reset value: 0x5400000000000153. */
static const uint64_t kReset = 0x5400000000000153ULL;

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return DEL_MODE_AUTO;
    case AC_MODE_COOL: return DEL_MODE_COOL;
    case AC_MODE_DRY:  return DEL_MODE_DRY;
    case AC_MODE_FAN:  return DEL_MODE_FAN;
    /* No native heat: collapse to Auto. */
    default:           return DEL_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return DEL_FAN_AUTO;
    case AC_FAN_LOW:  return DEL_FAN_LOW;
    case AC_FAN_MED:  return DEL_FAN_MED;
    case AC_FAN_HIGH: return DEL_FAN_HIGH;
    default:          return DEL_FAN_AUTO;
    }
}

/* Sum bytes 0..6, store as byte 7. */
static uint8_t calc_checksum(uint64_t state)
{
    uint8_t sum = 0;
    for(int off = 0; off < DEL_CHECKSUM_BIT; off += 8) {
        sum += (uint8_t)((state >> off) & 0xFF);
    }
    return sum;
}

static uint64_t apply_state(const AcState *st)
{
    uint64_t s = kReset;

    uint8_t temp = st->temp_c;
    if(temp < DEL_MIN_TEMP) temp = DEL_MIN_TEMP;
    if(temp > DEL_MAX_TEMP) temp = DEL_MAX_TEMP;
    /* Auto/Dry override temp; Fan mode uses sentinel 0b00110. */
    uint8_t mode = map_mode(st->mode);
    uint8_t tval;
    if(mode == DEL_MODE_AUTO || mode == DEL_MODE_DRY) tval = 0;
    else if(mode == DEL_MODE_FAN)                     tval = 0b00110;
    else                                              tval = (uint8_t)(temp - DEL_MIN_TEMP + 1);

    /* Byte 1 (bits 8..15): Temp(5) | Fan(2) | Fahrenheit(1). Clear & set. */
    s &= ~((uint64_t)0xFF << 8);
    s |= ((uint64_t)tval & 0x1F) << 8;
    s |= ((uint64_t)map_fan(st->fan) & 0x3) << (8 + 5);

    /* Byte 2 (bits 16..23): Power(1) | Mode(3) | Boost(1) | Sleep(1) | _(2). */
    s &= ~((uint64_t)0xFF << 16);
    if(st->power) s |= (uint64_t)1 << 16;
    s |= ((uint64_t)mode & 0x7) << 17;

    /* Sum bits 56..63. */
    s &= ~((uint64_t)0xFF << DEL_CHECKSUM_BIT);
    s |= ((uint64_t)calc_checksum(s)) << DEL_CHECKSUM_BIT;
    return s;
}

static esp_err_t delonghi_encode(const AcState *st,
                                 uint16_t **out_t, size_t *out_n,
                                 uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint64_t state = apply_state(st);

    /* hdr(2) + 8*16 + footer(2) = 132. */
    const size_t cap = 160;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = DEL_HDR_MARK;
    buf[n++] = DEL_HDR_SPACE;
    for(int i = 0; i < DEL_BYTES; i++) {
        uint8_t b = (uint8_t)((state >> (i * 8)) & 0xFF);
        ac_push_byte_lsb(buf, cap, &n, b,
                         DEL_BIT_MARK, DEL_ONE_SPACE, DEL_ZERO_SPACE);
    }
    buf[n++] = DEL_BIT_MARK;
    buf[n++] = DEL_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = DEL_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_delonghi = {
    .name   = "Delonghi AC",
    .encode = delonghi_encode,
};
