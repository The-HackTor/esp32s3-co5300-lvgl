#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define ARGO_BYTES         12
#define ARGO_FREQ_HZ       38000

#define ARGO_HDR_MARK      6400
#define ARGO_HDR_SPACE     3300
#define ARGO_BIT_MARK      400
#define ARGO_ONE_SPACE     2200
#define ARGO_ZERO_SPACE    900
/* Upstream uses kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define ARGO_GAP           30000

#define ARGO_PREAMBLE1     0xAC
#define ARGO_PREAMBLE2     0xF5
#define ARGO_POST          0b10
#define ARGO_TEMP_DELTA    4

#define ARGO_MIN_TEMP      16
#define ARGO_MAX_TEMP      30

#define ARGO_MODE_COOL     0
#define ARGO_MODE_DRY      1
#define ARGO_MODE_AUTO     2
#define ARGO_MODE_OFF      3
#define ARGO_MODE_HEAT     4

#define ARGO_FAN_AUTO      0
#define ARGO_FAN_1         1
#define ARGO_FAN_2         2
#define ARGO_FAN_3         3

#define ARGO_FLAP_AUTO     0
#define ARGO_FLAP_FULL     7

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return ARGO_MODE_AUTO;
    case AC_MODE_COOL: return ARGO_MODE_COOL;
    case AC_MODE_DRY:  return ARGO_MODE_DRY;
    case AC_MODE_HEAT: return ARGO_MODE_HEAT;
    case AC_MODE_FAN:  return ARGO_MODE_OFF;   /* WREM-2 maps Fan to "Off" mode. */
    default:           return ARGO_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return ARGO_FAN_AUTO;
    case AC_FAN_LOW:  return ARGO_FAN_1;
    case AC_FAN_MED:  return ARGO_FAN_2;
    case AC_FAN_HIGH: return ARGO_FAN_3;
    default:          return ARGO_FAN_AUTO;
    }
}

/* Write `bits` LSB-first into `s` starting at absolute bit offset `off`. */
static void put_bits(uint8_t *s, size_t off, uint8_t value, uint8_t bits)
{
    for(uint8_t i = 0; i < bits; i++) {
        size_t b = off + i;
        uint8_t v = (uint8_t)((value >> i) & 1);
        s[b >> 3] = (uint8_t)((s[b >> 3] & ~(1u << (b & 7)))
                            | (v << (b & 7)));
    }
}

static void apply_state(uint8_t s[ARGO_BYTES], const AcState *st)
{
    memset(s, 0, ARGO_BYTES);
    s[0] = ARGO_PREAMBLE1;
    s[1] = ARGO_PREAMBLE2;

    uint8_t temp = st->temp_c;
    if(temp < ARGO_MIN_TEMP) temp = ARGO_MIN_TEMP;
    if(temp > ARGO_MAX_TEMP) temp = ARGO_MAX_TEMP;

    /* byte 2 bit 3..5 = Mode, byte 2 bit 6 .. byte 3 bit 2 = Temp(5),
     * byte 3 bit 3..4 = Fan, byte 4 bit 2..4 = Flap, byte 9 bit 5 = Power. */
    put_bits(s, 16 + 3, map_mode(st->mode), 3);
    put_bits(s, 16 + 6, (uint8_t)(temp - ARGO_TEMP_DELTA), 5);
    put_bits(s, 24 + 3, map_fan(st->fan), 2);
    put_bits(s, 32 + 2,
             (uint8_t)(st->swing ? ARGO_FLAP_FULL : ARGO_FLAP_AUTO), 3);
    put_bits(s, 72 + 5, st->power ? 1 : 0, 1);

    /* Post (0b10) at byte 10 bits 0..1. */
    put_bits(s, 80, ARGO_POST, 2);

    /* Checksum = sumBytes(state[0..9], init=2); spans byte 10 bits 2..7 +
     * byte 11 bits 0..1 (8 bits LSB-first). */
    uint8_t sum = 2;
    for(int i = 0; i < 10; i++) sum += s[i];
    put_bits(s, 80 + 2, sum, 8);
}

static esp_err_t argo_encode(const AcState *st,
                             uint16_t **out_t, size_t *out_n,
                             uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[ARGO_BYTES];
    apply_state(s, st);

    /* hdr(2) + 12*16 + footer(2) = 196. */
    const size_t cap = 240;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = ARGO_HDR_MARK;
    buf[n++] = ARGO_HDR_SPACE;
    for(int i = 0; i < ARGO_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         ARGO_BIT_MARK, ARGO_ONE_SPACE, ARGO_ZERO_SPACE);
    }
    buf[n++] = ARGO_BIT_MARK;
    buf[n++] = ARGO_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = ARGO_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_argo = {
    .name   = "Argo AC",
    .encode = argo_encode,
};
