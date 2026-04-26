#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define HITACHI_BYTES      28
#define HITACHI_FREQ_HZ    38000

#define HITACHI_HDR_MARK   3300
#define HITACHI_HDR_SPACE  1700
#define HITACHI_BIT_MARK   400
#define HITACHI_ONE_SPACE  1250
#define HITACHI_ZERO_SPACE 500
/* Upstream gap kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define HITACHI_GAP        30000

#define HITACHI_MIN_TEMP   16
#define HITACHI_MAX_TEMP   32

#define HITACHI_MODE_AUTO  2
#define HITACHI_MODE_HEAT  3
#define HITACHI_MODE_COOL  4
#define HITACHI_MODE_DRY   5
#define HITACHI_MODE_FAN   0xC

#define HITACHI_FAN_AUTO   1
#define HITACHI_FAN_LOW    2
#define HITACHI_FAN_MED    3
#define HITACHI_FAN_HIGH   5

static uint8_t reverse8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

static const uint8_t kReset[HITACHI_BYTES] = {
    0x80, 0x08, 0x0C, 0x02, 0xFD, 0x80, 0x7F, 0x88,
    0x48, 0x10, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return HITACHI_MODE_AUTO;
    case AC_MODE_COOL: return HITACHI_MODE_COOL;
    case AC_MODE_DRY:  return HITACHI_MODE_DRY;
    case AC_MODE_HEAT: return HITACHI_MODE_HEAT;
    case AC_MODE_FAN:  return HITACHI_MODE_FAN;
    default:           return HITACHI_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f, uint8_t mode)
{
    /* Dry mode only allows Low or Low+1; Fan mode disallows Auto. */
    if(mode == HITACHI_MODE_DRY) return HITACHI_FAN_LOW;
    switch(f) {
    case AC_FAN_AUTO: return (mode == HITACHI_MODE_FAN) ? HITACHI_FAN_LOW
                                                        : HITACHI_FAN_AUTO;
    case AC_FAN_LOW:  return HITACHI_FAN_LOW;
    case AC_FAN_MED:  return HITACHI_FAN_MED;
    case AC_FAN_HIGH: return HITACHI_FAN_HIGH;
    default:          return HITACHI_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[HITACHI_BYTES], const AcState *st)
{
    uint8_t mode = map_mode(st->mode);
    uint8_t fan  = map_fan(st->fan, mode);

    uint8_t temp = st->temp_c;
    if(temp < HITACHI_MIN_TEMP) temp = HITACHI_MIN_TEMP;
    if(temp > HITACHI_MAX_TEMP) temp = HITACHI_MAX_TEMP;
    /* Fan mode uses sentinel 64 for temp. */
    uint8_t temp_field = (mode == HITACHI_MODE_FAN) ? 64 : temp;

    /* Bytes are stored bit-reversed because transmission is MSB-first;
     * pre-reversal yields the natural LSB-first wire pattern. */
    s[10] = reverse8(mode);
    s[11] = reverse8((uint8_t)(temp_field << 1));
    s[13] = reverse8(fan);
    s[14] = (uint8_t)((s[14] & 0x7F) | (st->swing ? 0x80 : 0x00));
    s[17] = (uint8_t)((s[17] & 0xFE) | (st->power ? 0x01 : 0x00));
    s[9]  = (temp == HITACHI_MIN_TEMP) ? 0x90 : 0x10;

    /* Checksum byte 27: 62 - sum(reverseBits(state[i])), then reverse the result. */
    uint8_t sum = 62;
    for(int i = 0; i < HITACHI_BYTES - 1; i++) sum -= reverse8(s[i]);
    s[HITACHI_BYTES - 1] = reverse8(sum);
}

static void push_byte_msb(uint16_t *buf, size_t cap, size_t *n, uint8_t b)
{
    for(int bit = 7; bit >= 0; bit--) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = HITACHI_BIT_MARK;
        buf[(*n)++] = (b & (1 << bit)) ? HITACHI_ONE_SPACE : HITACHI_ZERO_SPACE;
    }
}

static esp_err_t hitachi_encode(const AcState *st,
                                uint16_t **out_t, size_t *out_n,
                                uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[HITACHI_BYTES];
    memcpy(s, kReset, HITACHI_BYTES);
    apply_state(s, st);

    /* header(2) + 28*16 + footer(2) = 452. */
    const size_t cap = 500;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = HITACHI_HDR_MARK;
    buf[n++] = HITACHI_HDR_SPACE;
    for(int i = 0; i < HITACHI_BYTES; i++) {
        push_byte_msb(buf, cap, &n, s[i]);
    }
    buf[n++] = HITACHI_BIT_MARK;
    buf[n++] = HITACHI_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = HITACHI_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_hitachi = {
    .name   = "Hitachi AC",
    .encode = hitachi_encode,
};
