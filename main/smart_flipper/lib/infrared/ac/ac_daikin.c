#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define DAIKIN_BYTES        35
#define DAIKIN_S1_LEN       8
#define DAIKIN_S2_LEN       8
#define DAIKIN_S3_LEN       (DAIKIN_BYTES - DAIKIN_S1_LEN - DAIKIN_S2_LEN)
#define DAIKIN_HEADER_BITS  5

#define DAIKIN_HDR_MARK     3650
#define DAIKIN_HDR_SPACE    1623
#define DAIKIN_BIT_MARK     428
#define DAIKIN_ZERO_SPACE   428
#define DAIKIN_ONE_SPACE    1280
#define DAIKIN_GAP          29000
#define DAIKIN_FREQ_HZ      38000

#define DAIKIN_MIN_TEMP     10
#define DAIKIN_MAX_TEMP     32

static const uint8_t kReset[DAIKIN_BYTES] = {
    0x11, 0xDA, 0x27, 0x00, 0xC5, 0x00, 0x00, 0x00,
    0x11, 0xDA, 0x27, 0x00, 0x42, 0x00, 0x00, 0x00,
    0x11, 0xDA, 0x27, 0x00, 0x00, 0x49, 0x1E, 0x00,
    0xB0, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0xC0,
    0x00, 0x00, 0x00,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return 0;     /* 0b000 */
    case AC_MODE_DRY:  return 2;     /* 0b010 */
    case AC_MODE_COOL: return 3;     /* 0b011 */
    case AC_MODE_HEAT: return 4;     /* 0b100 */
    case AC_MODE_FAN:  return 6;     /* 0b110 */
    default:           return 0;
    }
}

/* IRremoteESP8266 stores fan as: Auto=0xA, Quiet=0xB, otherwise 2+speed.
 * We map AC_FAN_LOW=1->3, AC_FAN_MED=3->5, AC_FAN_HIGH=5->7. */
static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return 0xA;
    case AC_FAN_LOW:  return 3;
    case AC_FAN_MED:  return 5;
    case AC_FAN_HIGH: return 7;
    default:          return 0xA;
    }
}

static void apply_state(uint8_t s[DAIKIN_BYTES], const AcState *st)
{
    uint8_t temp = st->temp_c;
    if(temp < DAIKIN_MIN_TEMP) temp = DAIKIN_MIN_TEMP;
    if(temp > DAIKIN_MAX_TEMP) temp = DAIKIN_MAX_TEMP;

    /* Byte 21: bit0 Power, bit3 always 1, bits4..6 Mode. */
    s[21] = (uint8_t)((st->power ? 0x01 : 0x00)
                    | 0x08
                    | ((map_mode(st->mode) & 0x07) << 4));

    /* Byte 22: temperature in half-degrees (Temp:6 starting at bit0).
     * Daikin stores temp*2 in 6 bits = 0..63, covering up to 31.5C. */
    s[22] = (uint8_t)(temp * 2);

    /* Byte 24: low nibble SwingV (0xF on, 0x0 off), high nibble Fan. */
    uint8_t swing = st->swing ? 0x0F : 0x00;
    s[24] = (uint8_t)((map_fan(st->fan) << 4) | swing);
}

static void compute_checksums(uint8_t s[DAIKIN_BYTES])
{
    s[7]  = ac_sum_bytes(s,                     DAIKIN_S1_LEN - 1);
    s[15] = ac_sum_bytes(s + DAIKIN_S1_LEN,     DAIKIN_S2_LEN - 1);
    s[34] = ac_sum_bytes(s + DAIKIN_S1_LEN + DAIKIN_S2_LEN, DAIKIN_S3_LEN - 1);
}

static void emit_section(uint16_t *buf, size_t cap, size_t *n,
                         const uint8_t *bytes, size_t len)
{
    if(*n + 2 > cap) return;
    buf[(*n)++] = DAIKIN_HDR_MARK;
    buf[(*n)++] = DAIKIN_HDR_SPACE;
    for(size_t i = 0; i < len; i++) {
        ac_push_byte_lsb(buf, cap, n, bytes[i],
                         DAIKIN_BIT_MARK, DAIKIN_ONE_SPACE, DAIKIN_ZERO_SPACE);
    }
    if(*n + 2 > cap) return;
    buf[(*n)++] = DAIKIN_BIT_MARK;
    buf[(*n)++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;
}

static esp_err_t daikin_encode(const AcState *st,
                               uint16_t **out_t, size_t *out_n,
                               uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[DAIKIN_BYTES];
    memcpy(s, kReset, DAIKIN_BYTES);
    apply_state(s, st);
    compute_checksums(s);

    /* 5-bit pre-header (10) + 3 sections * (header(2) + bytes*16 + footer(2)).
     * Worst case: 10 + (4 + 8*16) + (4 + 8*16) + (4 + 19*16) = 10 + 132 + 132 + 308 = 582. */
    const size_t cap = 700;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    /* 5-bit header: all zero bits, no preamble header, plain bit-mark + zero-space. */
    for(int i = 0; i < DAIKIN_HEADER_BITS; i++) {
        buf[n++] = DAIKIN_BIT_MARK;
        buf[n++] = DAIKIN_ZERO_SPACE;
    }
    buf[n++] = DAIKIN_BIT_MARK;
    buf[n++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;

    emit_section(buf, cap, &n, s,                         DAIKIN_S1_LEN);
    emit_section(buf, cap, &n, s + DAIKIN_S1_LEN,         DAIKIN_S2_LEN);
    emit_section(buf, cap, &n, s + DAIKIN_S1_LEN + DAIKIN_S2_LEN, DAIKIN_S3_LEN);

    *out_t    = buf;
    *out_n    = n;
    *out_freq = DAIKIN_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_daikin = {
    .name   = "Daikin AC",
    .encode = daikin_encode,
};
