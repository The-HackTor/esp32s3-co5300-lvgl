#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define MIDEA_BYTES        6
#define MIDEA_FREQ_HZ      38000

#define MIDEA_TICK         80
#define MIDEA_HDR_MARK     (56 * MIDEA_TICK)   /* 4480 */
#define MIDEA_HDR_SPACE    (56 * MIDEA_TICK)   /* 4480 */
#define MIDEA_BIT_MARK     (7  * MIDEA_TICK)   /* 560  */
#define MIDEA_ONE_SPACE    (21 * MIDEA_TICK)   /* 1680 */
#define MIDEA_ZERO_SPACE   (7  * MIDEA_TICK)   /* 560  */
#define MIDEA_MIN_GAP      ((56 + 7 + 7) * MIDEA_TICK)  /* 5600 */
/* Upstream uses kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define MIDEA_MSG_GAP      30000

#define MIDEA_MIN_TEMP     17
#define MIDEA_MAX_TEMP     30

#define MIDEA_MODE_COOL    0
#define MIDEA_MODE_DRY     1
#define MIDEA_MODE_AUTO    2
#define MIDEA_MODE_HEAT    3
#define MIDEA_MODE_FAN     4

#define MIDEA_FAN_AUTO     0
#define MIDEA_FAN_LOW      1
#define MIDEA_FAN_MED      2
#define MIDEA_FAN_HIGH     3

#define MIDEA_TYPE_CMD     0b001
#define MIDEA_HEADER       0b10100

static uint8_t reverse8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return MIDEA_MODE_AUTO;
    case AC_MODE_COOL: return MIDEA_MODE_COOL;
    case AC_MODE_DRY:  return MIDEA_MODE_DRY;
    case AC_MODE_FAN:  return MIDEA_MODE_FAN;
    case AC_MODE_HEAT: return MIDEA_MODE_HEAT;
    default:           return MIDEA_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return MIDEA_FAN_AUTO;
    case AC_FAN_LOW:  return MIDEA_FAN_LOW;
    case AC_FAN_MED:  return MIDEA_FAN_MED;
    case AC_FAN_HIGH: return MIDEA_FAN_HIGH;
    default:          return MIDEA_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[MIDEA_BYTES], const AcState *st)
{
    /* Reset = 0xA1826FFFFF62 (Power On, Auto, Auto, 25C). LSB-byte first. */
    static const uint8_t kReset[MIDEA_BYTES] = {
        0x62, 0xFF, 0xFF, 0x6F, 0x82, 0xA1,
    };
    memcpy(s, kReset, MIDEA_BYTES);

    uint8_t temp = st->temp_c;
    if(temp < MIDEA_MIN_TEMP) temp = MIDEA_MIN_TEMP;
    if(temp > MIDEA_MAX_TEMP) temp = MIDEA_MAX_TEMP;

    /* byte 3: Temp(5) at bits 0..4 (degrees - MinTemp), bit 5 = useFahrenheit. */
    s[3] = (uint8_t)(temp - MIDEA_MIN_TEMP);

    /* byte 4: Mode(3) Fan(2) :1 Sleep(1) Power(1). */
    s[4] = (uint8_t)((map_mode(st->mode) & 0x07)
                  | ((map_fan(st->fan) & 0x03) << 3)
                  | (st->power ? (1 << 7) : 0));

    /* byte 5: Type(3) Header(5). Force a normal Command frame. */
    s[5] = (uint8_t)(MIDEA_TYPE_CMD | (MIDEA_HEADER << 3));

    /* Sensor disabled when not in FollowMe. */
    s[1] = 0xFF;
    s[2] = 0xFF;

    /* Checksum: sum reverseBits(byte[1..5]); sum = 256-sum; reverseBits(sum). */
    uint8_t sum = 0;
    for(int i = 1; i < MIDEA_BYTES; i++) sum += reverse8(s[i]);
    sum = (uint8_t)(256 - sum);
    s[0] = reverse8(sum);
}

static void push_byte_msb(uint16_t *buf, size_t cap, size_t *n, uint8_t b)
{
    for(int bit = 7; bit >= 0; bit--) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = MIDEA_BIT_MARK;
        buf[(*n)++] = (b & (1 << bit)) ? MIDEA_ONE_SPACE : MIDEA_ZERO_SPACE;
    }
}

static esp_err_t midea_encode(const AcState *st,
                              uint16_t **out_t, size_t *out_n,
                              uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[MIDEA_BYTES];
    apply_state(s, st);

    /* 2 phases of [hdr(2) + 6*16 + footer(2)] + final gap = 2*100 + 2 ~ 202. */
    const size_t cap = 240;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;

    /* Phase 1: send state MSB-first, byte 5 (header) first down to byte 0 (sum). */
    buf[n++] = MIDEA_HDR_MARK;
    buf[n++] = MIDEA_HDR_SPACE;
    for(int i = MIDEA_BYTES - 1; i >= 0; i--) {
        push_byte_msb(buf, cap, &n, s[i]);
    }
    buf[n++] = MIDEA_BIT_MARK;
    buf[n++] = MIDEA_MIN_GAP;

    /* Phase 2: same payload bit-inverted. */
    buf[n++] = MIDEA_HDR_MARK;
    buf[n++] = MIDEA_HDR_SPACE;
    for(int i = MIDEA_BYTES - 1; i >= 0; i--) {
        push_byte_msb(buf, cap, &n, (uint8_t)~s[i]);
    }
    buf[n++] = MIDEA_BIT_MARK;
    buf[n++] = MIDEA_MSG_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = MIDEA_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_midea = {
    .name   = "Midea AC",
    .encode = midea_encode,
};
