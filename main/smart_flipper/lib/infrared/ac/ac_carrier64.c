#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define CARR64_BYTES        8
#define CARR64_FREQ_HZ      38000

#define CARR64_HDR_MARK     8940
#define CARR64_HDR_SPACE    4556
#define CARR64_BIT_MARK     503
#define CARR64_ONE_SPACE    1736
#define CARR64_ZERO_SPACE   615
/* Upstream uses kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define CARR64_GAP          30000

#define CARR64_MIN_TEMP     16
#define CARR64_MAX_TEMP     30

#define CARR64_MODE_HEAT    0b01
#define CARR64_MODE_COOL    0b10
#define CARR64_MODE_FAN     0b11

#define CARR64_FAN_AUTO     0b00
#define CARR64_FAN_LOW      0b01
#define CARR64_FAN_MED      0b10
#define CARR64_FAN_HIGH     0b11

/* Reset value: 0x109000002C2A5584 (LSB byte 0 = 0x84). */
static const uint64_t kReset = 0x109000002C2A5584ULL;

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_HEAT: return CARR64_MODE_HEAT;
    case AC_MODE_COOL: return CARR64_MODE_COOL;
    case AC_MODE_FAN:  return CARR64_MODE_FAN;
    /* No native Auto/Dry: collapse to Cool. */
    default:           return CARR64_MODE_COOL;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return CARR64_FAN_AUTO;
    case AC_FAN_LOW:  return CARR64_FAN_LOW;
    case AC_FAN_MED:  return CARR64_FAN_MED;
    case AC_FAN_HIGH: return CARR64_FAN_HIGH;
    default:          return CARR64_FAN_AUTO;
    }
}

/* Sum nibbles from bit 20..63; result is the 4-bit Sum at bits 16..19. */
static uint8_t calc_checksum(uint64_t state)
{
    uint64_t data = state >> 20;
    uint8_t result = 0;
    for(; data; data >>= 4) result += (uint8_t)(data & 0xF);
    return result & 0xF;
}

static uint64_t apply_state(const AcState *st)
{
    uint64_t s = kReset;
    uint8_t temp = st->temp_c;
    if(temp < CARR64_MIN_TEMP) temp = CARR64_MIN_TEMP;
    if(temp > CARR64_MAX_TEMP) temp = CARR64_MAX_TEMP;

    uint8_t mode = map_mode(st->mode);
    uint8_t fan  = map_fan(st->fan);
    uint8_t tval = (uint8_t)(temp - CARR64_MIN_TEMP);

    /* Byte 2 (bits 16..23): Sum(4) | Mode(2) | Fan(2) -- clear all then set. */
    s &= ~((uint64_t)0xFF << 16);
    s |= ((uint64_t)mode & 0x3) << (16 + 4);
    s |= ((uint64_t)fan  & 0x3) << (16 + 6);

    /* Byte 3 (bits 24..31): Temp(4) | _(1) | SwingV(1) | _(2). */
    s &= ~((uint64_t)0xFF << 24);
    s |= ((uint64_t)tval & 0xF) << 24;
    if(st->swing) s |= (uint64_t)1 << (24 + 5);

    /* Byte 4 (bits 32..39): _(4) | Power(1) | ... clear power-region only. */
    s &= ~((uint64_t)0x10 << 32);
    if(st->power) s |= (uint64_t)1 << (32 + 4);

    /* Sum at bits 16..19. */
    s |= ((uint64_t)calc_checksum(s) & 0xF) << 16;
    return s;
}

static esp_err_t carrier64_encode(const AcState *st,
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
    buf[n++] = CARR64_HDR_MARK;
    buf[n++] = CARR64_HDR_SPACE;
    for(int i = 0; i < CARR64_BYTES; i++) {
        uint8_t b = (uint8_t)((state >> (i * 8)) & 0xFF);
        ac_push_byte_lsb(buf, cap, &n, b,
                         CARR64_BIT_MARK, CARR64_ONE_SPACE, CARR64_ZERO_SPACE);
    }
    buf[n++] = CARR64_BIT_MARK;
    buf[n++] = CARR64_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = CARR64_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_carrier64 = {
    .name   = "Carrier64 AC",
    .encode = carrier64_encode,
};
