#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define TOSHIBA_BYTES      9
#define TOSHIBA_FREQ_HZ    38000

#define TOSHIBA_HDR_MARK   4400
#define TOSHIBA_HDR_SPACE  4300
#define TOSHIBA_BIT_MARK   580
#define TOSHIBA_ONE_SPACE  1600
#define TOSHIBA_ZERO_SPACE 490
#define TOSHIBA_GAP        7400

#define TOSHIBA_MIN_TEMP   17
#define TOSHIBA_MAX_TEMP   30

#define TOSHIBA_MODE_AUTO  0
#define TOSHIBA_MODE_COOL  1
#define TOSHIBA_MODE_DRY   2
#define TOSHIBA_MODE_HEAT  3
#define TOSHIBA_MODE_FAN   4
#define TOSHIBA_MODE_OFF   7

#define TOSHIBA_FAN_AUTO   0
#define TOSHIBA_FAN_MIN    2
#define TOSHIBA_FAN_MED    4
#define TOSHIBA_FAN_MAX    6

static const uint8_t kReset[TOSHIBA_BYTES] = {
    0xF2, 0x0D, 0x03, 0xFC, 0x01, 0x00, 0x00, 0x00, 0x00,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return TOSHIBA_MODE_AUTO;
    case AC_MODE_COOL: return TOSHIBA_MODE_COOL;
    case AC_MODE_DRY:  return TOSHIBA_MODE_DRY;
    case AC_MODE_HEAT: return TOSHIBA_MODE_HEAT;
    case AC_MODE_FAN:  return TOSHIBA_MODE_FAN;
    default:           return TOSHIBA_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return TOSHIBA_FAN_AUTO;
    case AC_FAN_LOW:  return TOSHIBA_FAN_MIN;
    case AC_FAN_MED:  return TOSHIBA_FAN_MED;
    case AC_FAN_HIGH: return TOSHIBA_FAN_MAX;
    default:          return TOSHIBA_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[TOSHIBA_BYTES], const AcState *st)
{
    uint8_t temp = st->temp_c;
    if(temp < TOSHIBA_MIN_TEMP) temp = TOSHIBA_MIN_TEMP;
    if(temp > TOSHIBA_MAX_TEMP) temp = TOSHIBA_MAX_TEMP;

    /* Byte 5: bits 0..2 Swing, bits 4..7 Temp(deg-17). */
    s[5] = (uint8_t)(((temp - TOSHIBA_MIN_TEMP) & 0x0F) << 4)
         | (uint8_t)(st->swing ? 0x01 : 0x02);

    /* Byte 6: bits 0..2 Mode, bits 5..7 Fan. Power off = Mode 7. */
    uint8_t mode = st->power ? map_mode(st->mode) : TOSHIBA_MODE_OFF;
    s[6] = (uint8_t)((mode & 0x07) | ((map_fan(st->fan) & 0x07) << 5));

    /* Bytes 0..3 are inverted-byte pairs; refresh bytes 1 and 3 from 0 and 2. */
    s[1] = (uint8_t)~s[0];
    s[3] = (uint8_t)~s[2];

    /* Byte 8: XOR of bytes 0..7. */
    uint8_t x = 0;
    for(int i = 0; i < TOSHIBA_BYTES - 1; i++) x ^= s[i];
    s[TOSHIBA_BYTES - 1] = x;
}

static void push_byte_msb(uint16_t *buf, size_t cap, size_t *n, uint8_t b)
{
    for(int bit = 7; bit >= 0; bit--) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = TOSHIBA_BIT_MARK;
        buf[(*n)++] = (b & (1 << bit)) ? TOSHIBA_ONE_SPACE : TOSHIBA_ZERO_SPACE;
    }
}

static esp_err_t toshiba_encode(const AcState *st,
                                uint16_t **out_t, size_t *out_n,
                                uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[TOSHIBA_BYTES];
    memcpy(s, kReset, TOSHIBA_BYTES);
    apply_state(s, st);

    /* header(2) + 9*16 bits + footer(2) = 148. */
    const size_t cap = 200;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = TOSHIBA_HDR_MARK;
    buf[n++] = TOSHIBA_HDR_SPACE;
    for(int i = 0; i < TOSHIBA_BYTES; i++) {
        push_byte_msb(buf, cap, &n, s[i]);
    }
    buf[n++] = TOSHIBA_BIT_MARK;
    buf[n++] = TOSHIBA_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = TOSHIBA_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_toshiba = {
    .name   = "Toshiba AC",
    .encode = toshiba_encode,
};
