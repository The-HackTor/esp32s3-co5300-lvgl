#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

/* HitachiAc1 / HISENSE_AC variant: 13 bytes, MSB-first transmission. */
#define HISENSE_BYTES        13
#define HISENSE_FREQ_HZ      38000

#define HISENSE_HDR_MARK     3400
#define HISENSE_HDR_SPACE    3400
#define HISENSE_BIT_MARK     400
#define HISENSE_ONE_SPACE    1250
#define HISENSE_ZERO_SPACE   500
/* kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define HISENSE_GAP          30000

#define HISENSE_MIN_TEMP     16
#define HISENSE_MAX_TEMP     32
#define HISENSE_TEMP_DELTA   7
#define HISENSE_TEMP_AUTO    25

#define HISENSE_MODE_DRY     0b0010
#define HISENSE_MODE_FAN     0b0100
#define HISENSE_MODE_COOL    0b0110
#define HISENSE_MODE_HEAT    0b1001
#define HISENSE_MODE_AUTO    0b1110

#define HISENSE_FAN_AUTO     0b0001
#define HISENSE_FAN_HIGH     0b0010
#define HISENSE_FAN_MED      0b0100
#define HISENSE_FAN_LOW      0b1000

#define HISENSE_CHECKSUM_START 5

static uint8_t reverse_nibble(uint8_t n)
{
    n = (uint8_t)(((n & 0x8) >> 3) | ((n & 0x4) >> 1)
                | ((n & 0x2) << 1) | ((n & 0x1) << 3));
    return (uint8_t)(n & 0x0F);
}

static uint8_t reverse8(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

static uint8_t reverse_n(uint8_t v, uint8_t bits)
{
    uint8_t r = 0;
    for(uint8_t i = 0; i < bits; i++) {
        if(v & (1u << i)) r |= (uint8_t)(1u << (bits - 1 - i));
    }
    return r;
}

/* Known good reset state from upstream. */
static const uint8_t kReset[HISENSE_BYTES] = {
    0xB2, 0xAE, 0x4D, 0x91, 0xF0, 0xE1, 0xA4,
    0x00, 0x00, 0x00, 0x00, 0x61, 0x24,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return HISENSE_MODE_AUTO;
    case AC_MODE_COOL: return HISENSE_MODE_COOL;
    case AC_MODE_DRY:  return HISENSE_MODE_DRY;
    case AC_MODE_HEAT: return HISENSE_MODE_HEAT;
    case AC_MODE_FAN:  return HISENSE_MODE_FAN;
    default:           return HISENSE_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f, uint8_t mode)
{
    if(mode == HISENSE_MODE_AUTO) return HISENSE_FAN_AUTO;
    if(mode == HISENSE_MODE_DRY)  return HISENSE_FAN_LOW;
    switch(f) {
    case AC_FAN_AUTO: return (mode == HISENSE_MODE_HEAT || mode == HISENSE_MODE_FAN)
                                ? HISENSE_FAN_LOW : HISENSE_FAN_AUTO;
    case AC_FAN_LOW:  return HISENSE_FAN_LOW;
    case AC_FAN_MED:  return HISENSE_FAN_MED;
    case AC_FAN_HIGH: return HISENSE_FAN_HIGH;
    default:          return HISENSE_FAN_AUTO;
    }
}

/* Checksum: sum of reversed-nibbles bytes 5..(N-2), then reverse8. */
static uint8_t calc_checksum(const uint8_t *s)
{
    uint8_t sum = 0;
    for(int i = HISENSE_CHECKSUM_START; i < HISENSE_BYTES - 1; i++) {
        sum += reverse_nibble(s[i] & 0x0F);
        sum += reverse_nibble((s[i] >> 4) & 0x0F);
    }
    return reverse8(sum);
}

static void apply_state(uint8_t s[HISENSE_BYTES], const AcState *st)
{
    uint8_t mode = map_mode(st->mode);
    uint8_t fan  = map_fan(st->fan, mode);

    uint8_t temp = st->temp_c;
    if(mode == HISENSE_MODE_AUTO) temp = HISENSE_TEMP_AUTO;
    if(temp < HISENSE_MIN_TEMP)   temp = HISENSE_MIN_TEMP;
    if(temp > HISENSE_MAX_TEMP)   temp = HISENSE_MAX_TEMP;

    /* Byte 5: Mode(high nibble) | Fan(low nibble). */
    s[5] = (uint8_t)((mode << 4) | (fan & 0x0F));

    /* Byte 6: _(2) | Temp(5, LSB-stored) | _(1).
     * Upstream stores temp reversed in 5 bits at offset 2. */
    uint8_t tval = (uint8_t)((temp - HISENSE_TEMP_DELTA) & 0x1F);
    tval = reverse_n(tval, 5);
    s[6] = (uint8_t)((s[6] & ~(0x1F << 2)) | ((tval & 0x1F) << 2));

    /* Byte 11: SwingToggle(b0) | Sleep(b1..3) | PowerToggle(b4) | Power(b5)
     *          | SwingV(b6) | SwingH(b7). */
    if(st->power) s[11] |= (uint8_t)(1 << 5);
    else          s[11] &= (uint8_t)~(1 << 5);
    if(st->swing) s[11] |= (uint8_t)(1 << 6);
    else          s[11] &= (uint8_t)~(1 << 6);

    s[HISENSE_BYTES - 1] = calc_checksum(s);
}

static void push_byte_msb(uint16_t *buf, size_t cap, size_t *n, uint8_t b)
{
    for(int bit = 7; bit >= 0; bit--) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = HISENSE_BIT_MARK;
        buf[(*n)++] = (b & (1 << bit)) ? HISENSE_ONE_SPACE : HISENSE_ZERO_SPACE;
    }
}

static esp_err_t hisense_encode(const AcState *st,
                                uint16_t **out_t, size_t *out_n,
                                uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[HISENSE_BYTES];
    memcpy(s, kReset, HISENSE_BYTES);
    apply_state(s, st);

    /* hdr(2) + 13*16 + footer(2) = 212. */
    const size_t cap = 240;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = HISENSE_HDR_MARK;
    buf[n++] = HISENSE_HDR_SPACE;
    for(int i = 0; i < HISENSE_BYTES; i++) {
        push_byte_msb(buf, cap, &n, s[i]);
    }
    buf[n++] = HISENSE_BIT_MARK;
    buf[n++] = HISENSE_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = HISENSE_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_hisense = {
    .name   = "Hisense AC",
    .encode = hisense_encode,
};
