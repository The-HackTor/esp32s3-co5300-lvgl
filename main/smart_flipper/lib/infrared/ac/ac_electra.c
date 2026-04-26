#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define ELEC_BYTES         13
#define ELEC_FREQ_HZ       38000

#define ELEC_HDR_MARK      9166
#define ELEC_HDR_SPACE     4470
#define ELEC_BIT_MARK      646
#define ELEC_ONE_SPACE     1647
#define ELEC_ZERO_SPACE    547
/* kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define ELEC_GAP           30000

#define ELEC_MIN_TEMP      16
#define ELEC_MAX_TEMP      32
#define ELEC_TEMP_DELTA    8

#define ELEC_SWING_ON      0b000
#define ELEC_SWING_OFF     0b111

#define ELEC_FAN_AUTO      0b101
#define ELEC_FAN_LOW       0b011
#define ELEC_FAN_MED       0b010
#define ELEC_FAN_HIGH      0b001

#define ELEC_MODE_AUTO     0b000
#define ELEC_MODE_COOL     0b001
#define ELEC_MODE_DRY      0b010
#define ELEC_MODE_HEAT     0b100
#define ELEC_MODE_FAN      0b110

#define ELEC_LIGHT_OFF     0x08

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return ELEC_MODE_AUTO;
    case AC_MODE_COOL: return ELEC_MODE_COOL;
    case AC_MODE_DRY:  return ELEC_MODE_DRY;
    case AC_MODE_HEAT: return ELEC_MODE_HEAT;
    case AC_MODE_FAN:  return ELEC_MODE_FAN;
    default:           return ELEC_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return ELEC_FAN_AUTO;
    case AC_FAN_LOW:  return ELEC_FAN_LOW;
    case AC_FAN_MED:  return ELEC_FAN_MED;
    case AC_FAN_HIGH: return ELEC_FAN_HIGH;
    default:          return ELEC_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[ELEC_BYTES], const AcState *st)
{
    memset(s, 0, ELEC_BYTES);
    s[0]  = 0xC3;
    s[11] = ELEC_LIGHT_OFF;

    uint8_t temp = st->temp_c;
    if(temp < ELEC_MIN_TEMP) temp = ELEC_MIN_TEMP;
    if(temp > ELEC_MAX_TEMP) temp = ELEC_MAX_TEMP;
    uint8_t tval = (uint8_t)(temp - ELEC_TEMP_DELTA);

    /* Byte 1: SwingV(b0..2) | Temp(b3..7). */
    uint8_t swing = st->swing ? ELEC_SWING_ON : ELEC_SWING_OFF;
    s[1] = (uint8_t)((swing & 0x07) | ((tval & 0x1F) << 3));

    /* Byte 2: _(5) | SwingH(3) -- mirror swingv toggle as off (no horiz). */
    s[2] = (uint8_t)((ELEC_SWING_OFF & 0x07) << 5);

    /* Byte 4: _(5) | Fan(3). */
    s[4] = (uint8_t)((map_fan(st->fan) & 0x07) << 5);

    /* Byte 6: _(3) | IFeel(1) | _(1) | Mode(3). */
    s[6] = (uint8_t)((map_mode(st->mode) & 0x07) << 5);

    /* Byte 9: _(2) | Clean(1) | _(2) | Power(1) | _(2). */
    if(st->power) s[9] |= (uint8_t)(1 << 5);

    /* Byte 12: checksum = sumBytes(s[0..11]). */
    s[12] = ac_sum_bytes(s, ELEC_BYTES - 1);
}

static esp_err_t electra_encode(const AcState *st,
                                uint16_t **out_t, size_t *out_n,
                                uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[ELEC_BYTES];
    apply_state(s, st);

    /* hdr(2) + 13*16 + footer(2) = 212. */
    const size_t cap = 240;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = ELEC_HDR_MARK;
    buf[n++] = ELEC_HDR_SPACE;
    for(int i = 0; i < ELEC_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         ELEC_BIT_MARK, ELEC_ONE_SPACE, ELEC_ZERO_SPACE);
    }
    buf[n++] = ELEC_BIT_MARK;
    buf[n++] = ELEC_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = ELEC_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_electra = {
    .name   = "Electra AC",
    .encode = electra_encode,
};
