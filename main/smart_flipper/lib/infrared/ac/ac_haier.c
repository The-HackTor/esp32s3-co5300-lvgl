#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define HAIER_BYTES        9
#define HAIER_FREQ_HZ      38000

#define HAIER_HDR          3000
#define HAIER_HDR_GAP      4300
#define HAIER_BIT_MARK     520
#define HAIER_ONE_SPACE    1650
#define HAIER_ZERO_SPACE   650
/* Upstream kHaierAcMinGap = 150000us; cap to uint16_t-safe 30ms. */
#define HAIER_GAP          30000

#define HAIER_PREFIX       0xA5
#define HAIER_DEF_OFF_HRS  12

#define HAIER_MIN_TEMP     16
#define HAIER_MAX_TEMP     30

#define HAIER_MODE_AUTO    0
#define HAIER_MODE_COOL    1
#define HAIER_MODE_DRY     2
#define HAIER_MODE_HEAT    3
#define HAIER_MODE_FAN     4

/* Wire fan values (upstream stores Low=3, Med=2, High=1, Auto=0). */
#define HAIER_FAN_AUTO     0
#define HAIER_FAN_HIGH     1
#define HAIER_FAN_MED      2
#define HAIER_FAN_LOW      3

#define HAIER_SWING_OFF    0
#define HAIER_SWING_CHG    3

#define HAIER_CMD_ON       0x1

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return HAIER_MODE_AUTO;
    case AC_MODE_COOL: return HAIER_MODE_COOL;
    case AC_MODE_DRY:  return HAIER_MODE_DRY;
    case AC_MODE_HEAT: return HAIER_MODE_HEAT;
    case AC_MODE_FAN:  return HAIER_MODE_FAN;
    default:           return HAIER_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return HAIER_FAN_AUTO;
    case AC_FAN_LOW:  return HAIER_FAN_LOW;
    case AC_FAN_MED:  return HAIER_FAN_MED;
    case AC_FAN_HIGH: return HAIER_FAN_HIGH;
    default:          return HAIER_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[HAIER_BYTES], const AcState *st)
{
    memset(s, 0, HAIER_BYTES);
    s[0] = HAIER_PREFIX;
    s[2] = (1 << 5);                        /* unknown const = 1. */
    s[4] = HAIER_DEF_OFF_HRS & 0x1F;        /* default 12h off-timer. */

    uint8_t temp = st->temp_c;
    if(temp < HAIER_MIN_TEMP) temp = HAIER_MIN_TEMP;
    if(temp > HAIER_MAX_TEMP) temp = HAIER_MAX_TEMP;

    /* byte 1: Command(4 lo) + Temp(4 hi); store the On/power button. */
    s[1] = (uint8_t)((HAIER_CMD_ON & 0x0F)
                  | (((temp - HAIER_MIN_TEMP) & 0x0F) << 4));

    /* byte 2 high 2 bits = SwingV. */
    s[2] = (uint8_t)((s[2] & 0x3F)
                  | (((st->swing ? HAIER_SWING_CHG : HAIER_SWING_OFF)
                      & 0x03) << 6));

    /* byte 5 high 2 bits = Fan. */
    s[5] = (uint8_t)((map_fan(st->fan) & 0x03) << 6);

    /* byte 6 high 3 bits = Mode. */
    s[6] = (uint8_t)((map_mode(st->mode) & 0x07) << 5);

    /* No Power bit on this remote -- "off" = ignore frame. We still emit a
     * valid frame so downstream save/load works. */
    (void)st->power;

    s[HAIER_BYTES - 1] = ac_sum_bytes(s, HAIER_BYTES - 1);
}

static esp_err_t haier_encode(const AcState *st,
                              uint16_t **out_t, size_t *out_n,
                              uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[HAIER_BYTES];
    apply_state(s, st);

    /* double-hdr(4) + 9*16 + footer(2) = 150. */
    const size_t cap = 200;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    /* Haier emits a leading hdr+hdr pair *before* the standard hdr+gap. */
    buf[n++] = HAIER_HDR;
    buf[n++] = HAIER_HDR;
    buf[n++] = HAIER_HDR;
    buf[n++] = HAIER_HDR_GAP;
    for(int i = 0; i < HAIER_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         HAIER_BIT_MARK, HAIER_ONE_SPACE, HAIER_ZERO_SPACE);
    }
    buf[n++] = HAIER_BIT_MARK;
    buf[n++] = HAIER_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = HAIER_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_haier = {
    .name   = "Haier AC",
    .encode = haier_encode,
};
