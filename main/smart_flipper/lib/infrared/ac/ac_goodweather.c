#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define GW_BYTES         6     /* 48 bits */
#define GW_FREQ_HZ       38000

#define GW_HDR_MARK      6820
#define GW_HDR_SPACE     6820
#define GW_BIT_MARK      580
#define GW_ONE_SPACE     580
#define GW_ZERO_SPACE    1860
/* kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define GW_GAP           30000

#define GW_MIN_TEMP      16
#define GW_MAX_TEMP      31

#define GW_MODE_AUTO     0b000
#define GW_MODE_COOL     0b001
#define GW_MODE_DRY      0b010
#define GW_MODE_FAN      0b011
#define GW_MODE_HEAT     0b100

#define GW_FAN_AUTO      0b00
#define GW_FAN_HIGH      0b01
#define GW_FAN_MED       0b10
#define GW_FAN_LOW       0b11

#define GW_SWING_FAST    0b00
#define GW_SWING_OFF     0b10

#define GW_CMD_MODE      0x01

/* Initial state: 0xD50000000000 (byte 0 = 0x00 LSB ... byte 5 = 0xD5). */
static const uint64_t kReset = 0xD50000000000ULL;

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return GW_MODE_AUTO;
    case AC_MODE_COOL: return GW_MODE_COOL;
    case AC_MODE_DRY:  return GW_MODE_DRY;
    case AC_MODE_FAN:  return GW_MODE_FAN;
    case AC_MODE_HEAT: return GW_MODE_HEAT;
    default:           return GW_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return GW_FAN_AUTO;
    case AC_FAN_LOW:  return GW_FAN_LOW;
    case AC_FAN_MED:  return GW_FAN_MED;
    case AC_FAN_HIGH: return GW_FAN_HIGH;
    default:          return GW_FAN_AUTO;
    }
}

static uint64_t apply_state(const AcState *st)
{
    uint64_t s = kReset;

    uint8_t temp = st->temp_c;
    if(temp < GW_MIN_TEMP) temp = GW_MIN_TEMP;
    if(temp > GW_MAX_TEMP) temp = GW_MAX_TEMP;
    uint8_t tval = (uint8_t)(temp - GW_MIN_TEMP);

    /* Byte 2 (bits 16..23): Command(4) | _(4) -- set to Mode command. */
    s &= ~((uint64_t)0x0F << 16);
    s |= ((uint64_t)GW_CMD_MODE & 0x0F) << 16;

    /* Byte 3 (bits 24..31): Sleep(1) | Power(1) | Swing(2) | AirFlow(1) | Fan(2) | _(1). */
    s &= ~((uint64_t)0xFF << 24);
    if(st->power) s |= (uint64_t)1 << (24 + 1);
    {
        uint8_t sw = st->swing ? GW_SWING_FAST : GW_SWING_OFF;
        s |= ((uint64_t)sw & 0x3) << (24 + 2);
    }
    s |= ((uint64_t)map_fan(st->fan) & 0x3) << (24 + 5);

    /* Byte 4 (bits 32..39): Temp(4) | _(1) | Mode(3). */
    s &= ~((uint64_t)0xFF << 32);
    s |= ((uint64_t)tval & 0xF) << 32;
    s |= ((uint64_t)map_mode(st->mode) & 0x7) << (32 + 5);

    return s;
}

static void push_byte_with_inverse(uint16_t *buf, size_t cap, size_t *n,
                                   uint8_t b)
{
    /* Wire format: 8 LSB-first bits of byte then 8 LSB-first bits of ~byte. */
    ac_push_byte_lsb(buf, cap, n, b,
                     GW_BIT_MARK, GW_ONE_SPACE, GW_ZERO_SPACE);
    ac_push_byte_lsb(buf, cap, n, (uint8_t)~b,
                     GW_BIT_MARK, GW_ONE_SPACE, GW_ZERO_SPACE);
}

static esp_err_t goodweather_encode(const AcState *st,
                                    uint16_t **out_t, size_t *out_n,
                                    uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint64_t state = apply_state(st);

    /* hdr(2) + 6 bytes * 32 (16 wire bits) + footer(4) = 198. */
    const size_t cap = 240;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = GW_HDR_MARK;
    buf[n++] = GW_HDR_SPACE;
    for(int i = 0; i < GW_BYTES; i++) {
        uint8_t b = (uint8_t)((state >> (i * 8)) & 0xFF);
        push_byte_with_inverse(buf, cap, &n, b);
    }
    /* Footer: bit-mark + hdr-space + bit-mark + gap. */
    buf[n++] = GW_BIT_MARK;
    buf[n++] = GW_HDR_SPACE;
    buf[n++] = GW_BIT_MARK;
    buf[n++] = GW_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = GW_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_goodweather = {
    .name   = "GoodWeather AC",
    .encode = goodweather_encode,
};
