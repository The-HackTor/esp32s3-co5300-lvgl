#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define SAN_BYTES          9
#define SAN_FREQ_HZ        38000

#define SAN_HDR_MARK       8500
#define SAN_HDR_SPACE      4200
#define SAN_BIT_MARK       500
#define SAN_ONE_SPACE      1600
#define SAN_ZERO_SPACE     550
/* Upstream uses kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define SAN_GAP            30000

#define SAN_MIN_TEMP       16
#define SAN_MAX_TEMP       30
#define SAN_TEMP_DELTA     4

#define SAN_MODE_HEAT      1
#define SAN_MODE_COOL      2
#define SAN_MODE_DRY       3
#define SAN_MODE_AUTO      4

#define SAN_FAN_AUTO       0
#define SAN_FAN_HIGH       1
#define SAN_FAN_LOW        2
#define SAN_FAN_MED        3

#define SAN_POWER_OFF      0b01
#define SAN_POWER_ON       0b10

#define SAN_SWING_AUTO     0
#define SAN_SWING_FIXED    5      /* UpperMiddle (matches reset). */

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return SAN_MODE_AUTO;
    case AC_MODE_COOL: return SAN_MODE_COOL;
    case AC_MODE_DRY:  return SAN_MODE_DRY;
    case AC_MODE_HEAT: return SAN_MODE_HEAT;
    case AC_MODE_FAN:  return SAN_MODE_AUTO;   /* No Fan-only mode. */
    default:           return SAN_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return SAN_FAN_AUTO;
    case AC_FAN_LOW:  return SAN_FAN_LOW;
    case AC_FAN_MED:  return SAN_FAN_MED;
    case AC_FAN_HIGH: return SAN_FAN_HIGH;
    default:          return SAN_FAN_AUTO;
    }
}

/* sumNibbles over state[0..len-2]. */
static uint8_t sum_nibbles(const uint8_t *p, size_t n)
{
    uint8_t sum = 0;
    for(size_t i = 0; i < n; i++) sum += (p[i] >> 4) + (p[i] & 0x0F);
    return sum;
}

static void apply_state(uint8_t s[SAN_BYTES], const AcState *st)
{
    static const uint8_t kReset[SAN_BYTES] = {
        0x6A, 0x6D, 0x51, 0x00, 0x10, 0x45, 0x00, 0x00, 0x33,
    };
    memcpy(s, kReset, SAN_BYTES);

    uint8_t temp = st->temp_c;
    if(temp < SAN_MIN_TEMP) temp = SAN_MIN_TEMP;
    if(temp > SAN_MAX_TEMP) temp = SAN_MAX_TEMP;

    /* byte 1: Temp(5) :3 -- stored = degrees - 4. */
    s[1] = (uint8_t)((s[1] & ~0x1F) | ((temp - SAN_TEMP_DELTA) & 0x1F));

    /* byte 4: Fan(2) OffTimer(1) :1 Mode(3) :1 */
    s[4] = (uint8_t)((s[4] & ~0x77)
                   | (map_fan(st->fan) & 0x03)
                   | ((map_mode(st->mode) & 0x07) << 4));

    /* byte 5: SwingV(3) :3 Power(2) */
    uint8_t swingv = st->swing ? SAN_SWING_AUTO : SAN_SWING_FIXED;
    uint8_t power  = st->power ? SAN_POWER_ON   : SAN_POWER_OFF;
    s[5] = (uint8_t)((swingv & 0x07) | ((power & 0x03) << 6));

    /* byte 8: sumNibbles(state[0..7]). */
    s[8] = sum_nibbles(s, SAN_BYTES - 1);
}

static esp_err_t sanyo_encode(const AcState *st,
                              uint16_t **out_t, size_t *out_n,
                              uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[SAN_BYTES];
    apply_state(s, st);

    /* hdr(2) + 9*16 + footer(2) = 148. */
    const size_t cap = 180;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = SAN_HDR_MARK;
    buf[n++] = SAN_HDR_SPACE;
    for(int i = 0; i < SAN_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         SAN_BIT_MARK, SAN_ONE_SPACE, SAN_ZERO_SPACE);
    }
    buf[n++] = SAN_BIT_MARK;
    buf[n++] = SAN_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = SAN_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_sanyo = {
    .name   = "Sanyo AC",
    .encode = sanyo_encode,
};
