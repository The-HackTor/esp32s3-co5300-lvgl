#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define SHARP_BYTES        13
#define SHARP_FREQ_HZ      38000

#define SHARP_HDR_MARK     3800
#define SHARP_HDR_SPACE    1900
#define SHARP_BIT_MARK     470
#define SHARP_ONE_SPACE    1400
#define SHARP_ZERO_SPACE   500
/* Upstream gap kDefaultMessageGap = 100ms; uint16_t cap is 32767us. */
#define SHARP_GAP          30000

#define SHARP_MIN_TEMP     15
#define SHARP_MAX_TEMP     30

#define SHARP_POWER_OFF    0x2
#define SHARP_POWER_ON     0x3

#define SHARP_MODE_AUTO    0x0
#define SHARP_MODE_HEAT    0x1
#define SHARP_MODE_COOL    0x2
#define SHARP_MODE_DRY     0x3

#define SHARP_FAN_AUTO     0x2
#define SHARP_FAN_MIN      0x4
#define SHARP_FAN_MED      0x3
#define SHARP_FAN_HIGH     0x5
#define SHARP_FAN_MAX      0x7

static const uint8_t kReset[SHARP_BYTES] = {
    0xAA, 0x5A, 0xCF, 0x10, 0x00, 0x01, 0x00, 0x00,
    0x08, 0x80, 0x00, 0xE0, 0x01,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return SHARP_MODE_AUTO;
    case AC_MODE_COOL: return SHARP_MODE_COOL;
    case AC_MODE_DRY:  return SHARP_MODE_DRY;
    case AC_MODE_HEAT: return SHARP_MODE_HEAT;
    case AC_MODE_FAN:  return SHARP_MODE_AUTO;   /* A907 has no Fan-only mode */
    default:           return SHARP_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return SHARP_FAN_AUTO;
    case AC_FAN_LOW:  return SHARP_FAN_MIN;
    case AC_FAN_MED:  return SHARP_FAN_MED;
    case AC_FAN_HIGH: return SHARP_FAN_HIGH;
    default:          return SHARP_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[SHARP_BYTES], const AcState *st)
{
    uint8_t temp = st->temp_c;
    if(temp < SHARP_MIN_TEMP) temp = SHARP_MIN_TEMP;
    if(temp > SHARP_MAX_TEMP) temp = SHARP_MAX_TEMP;

    uint8_t mode = map_mode(st->mode);

    /* Byte 4: low nibble = Temp(deg-15). Auto/Dry have no temp; reset to 0xC0. */
    if(mode == SHARP_MODE_AUTO || mode == SHARP_MODE_DRY) {
        s[4] = 0x00;
    } else {
        s[4] = (uint8_t)(0xC0 | ((temp - SHARP_MIN_TEMP) & 0x0F));
    }

    /* Byte 5: high nibble = PowerSpecial. */
    s[5] = (uint8_t)((s[5] & 0x0F)
                  | (((st->power ? SHARP_POWER_ON : SHARP_POWER_OFF) & 0x0F) << 4));

    /* Byte 6: bits 0..1 Mode, bits 4..6 Fan. Auto/Dry force Fan=Auto. */
    uint8_t fan = (mode == SHARP_MODE_AUTO || mode == SHARP_MODE_DRY)
                  ? SHARP_FAN_AUTO : map_fan(st->fan);
    s[6] = (uint8_t)((s[6] & ~0x73)
                  | (mode & 0x03)
                  | ((fan & 0x07) << 4));

    /* Byte 8: bits 0..2 SwingV. Off = 0b010, Toggle = 0b111. */
    s[8] = (uint8_t)((s[8] & ~0x07) | (st->swing ? 0x07 : 0x02));

    /* Byte 10: Special; 0x00 = Power command. */
    s[10] = 0x00;

    /* Byte 12 high nibble = 4-bit XOR-fold checksum of state[0..11] + low(state[12]). */
    uint8_t x = 0;
    for(int i = 0; i < SHARP_BYTES - 1; i++) x ^= s[i];
    x ^= (uint8_t)(s[SHARP_BYTES - 1] & 0x0F);
    x ^= (uint8_t)(x >> 4);
    s[SHARP_BYTES - 1] = (uint8_t)((s[SHARP_BYTES - 1] & 0x0F) | ((x & 0x0F) << 4));
}

static esp_err_t sharp_encode(const AcState *st,
                              uint16_t **out_t, size_t *out_n,
                              uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[SHARP_BYTES];
    memcpy(s, kReset, SHARP_BYTES);
    apply_state(s, st);

    /* header(2) + 13*16 + footer(2) = 212. */
    const size_t cap = 250;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = SHARP_HDR_MARK;
    buf[n++] = SHARP_HDR_SPACE;
    for(int i = 0; i < SHARP_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         SHARP_BIT_MARK, SHARP_ONE_SPACE, SHARP_ZERO_SPACE);
    }
    buf[n++] = SHARP_BIT_MARK;
    buf[n++] = SHARP_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = SHARP_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_sharp = {
    .name   = "Sharp AC",
    .encode = sharp_encode,
};
