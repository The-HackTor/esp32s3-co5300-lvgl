#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define TCL_BYTES         14
#define TCL_FREQ_HZ       38000

#define TCL_HDR_MARK      3000
#define TCL_HDR_SPACE     1650
#define TCL_BIT_MARK      500
#define TCL_ONE_SPACE     1050
#define TCL_ZERO_SPACE    325
/* Upstream uses kDefaultMessageGap = 100ms; cap to uint16_t-safe 30ms. */
#define TCL_GAP           30000

#define TCL_MIN_TEMP      16
#define TCL_MAX_TEMP      31

#define TCL_MODE_HEAT     1
#define TCL_MODE_DRY      2
#define TCL_MODE_COOL     3
#define TCL_MODE_FAN      7
#define TCL_MODE_AUTO     8

#define TCL_FAN_AUTO      0b000
#define TCL_FAN_MIN       0b001
#define TCL_FAN_LOW       0b010
#define TCL_FAN_MED       0b011
#define TCL_FAN_HIGH      0b101

#define TCL_SWINGV_OFF    0b000
#define TCL_SWINGV_ON     0b111

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return TCL_MODE_AUTO;
    case AC_MODE_COOL: return TCL_MODE_COOL;
    case AC_MODE_DRY:  return TCL_MODE_DRY;
    case AC_MODE_FAN:  return TCL_MODE_FAN;
    case AC_MODE_HEAT: return TCL_MODE_HEAT;
    default:           return TCL_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return TCL_FAN_AUTO;
    case AC_FAN_LOW:  return TCL_FAN_LOW;
    case AC_FAN_MED:  return TCL_FAN_MED;
    case AC_FAN_HIGH: return TCL_FAN_HIGH;
    default:          return TCL_FAN_AUTO;
    }
}

static void apply_state(uint8_t s[TCL_BYTES], const AcState *st)
{
    static const uint8_t kReset[TCL_BYTES] = {
        0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03, 0x07,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x03,
    };
    memcpy(s, kReset, TCL_BYTES);

    uint8_t temp = st->temp_c;
    if(temp < TCL_MIN_TEMP) temp = TCL_MIN_TEMP;
    if(temp > TCL_MAX_TEMP) temp = TCL_MAX_TEMP;

    uint8_t mode = map_mode(st->mode);
    uint8_t fan  = map_fan(st->fan);
    /* Fan-only mode forces fan to High in upstream. */
    if(mode == TCL_MODE_FAN) fan = TCL_FAN_HIGH;

    /* byte 5: :2 Power(1) OffTimerEn(1) OnTimerEn(1) Quiet(1) Light(1) Econo(1). */
    s[5] = (uint8_t)((s[5] & ~(1 << 2)) | (st->power ? (1 << 2) : 0));

    /* byte 6: Mode(4) Health(1) Turbo(1) :2. */
    s[6] = (uint8_t)((s[6] & ~0x0F) | (mode & 0x0F));

    /* byte 7: Temp(4) :4 -- stored = 31 - degrees. Whole-degree only. */
    s[7] = (uint8_t)((s[7] & ~0x0F) | ((TCL_MAX_TEMP - temp) & 0x0F));

    /* byte 8: Fan(3) SwingV(3) TimerIndicator(1) :1. */
    uint8_t swing = st->swing ? TCL_SWINGV_ON : TCL_SWINGV_OFF;
    s[8] = (uint8_t)((s[8] & ~0x3F)
                  | (fan & 0x07)
                  | ((swing & 0x07) << 3));

    /* byte 12 bit 5: HalfDegree -- always 0 here. */
    s[12] = (uint8_t)(s[12] & ~(1 << 5));

    s[TCL_BYTES - 1] = ac_sum_bytes(s, TCL_BYTES - 1);
}

static esp_err_t tcl_encode(const AcState *st,
                            uint16_t **out_t, size_t *out_n,
                            uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[TCL_BYTES];
    apply_state(s, st);

    /* hdr(2) + 14*16 + footer(2) = 228. */
    const size_t cap = 260;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = TCL_HDR_MARK;
    buf[n++] = TCL_HDR_SPACE;
    for(int i = 0; i < TCL_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         TCL_BIT_MARK, TCL_ONE_SPACE, TCL_ZERO_SPACE);
    }
    buf[n++] = TCL_BIT_MARK;
    buf[n++] = TCL_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = TCL_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_tcl = {
    .name   = "TCL112 AC",
    .encode = tcl_encode,
};
