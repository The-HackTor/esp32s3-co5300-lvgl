#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define WPL_BYTES         21
#define WPL_FREQ_HZ       38000

#define WPL_HDR_MARK      8950
#define WPL_HDR_SPACE     4484
#define WPL_BIT_MARK      597
#define WPL_ONE_SPACE     1649
#define WPL_ZERO_SPACE    533
#define WPL_GAP           7920
/* Final gap is kDefaultMessageGap (100ms) upstream; hw_ir caps at 32767us. */
#define WPL_END_GAP       30000

#define WPL_MIN_TEMP      18
#define WPL_MAX_TEMP      32
#define WPL_AUTO_TEMP     23

#define WPL_MODE_HEAT     0
#define WPL_MODE_AUTO     1
#define WPL_MODE_COOL     2
#define WPL_MODE_DRY      3
#define WPL_MODE_FAN      4

#define WPL_FAN_AUTO      0
#define WPL_FAN_HIGH      1
#define WPL_FAN_MED       2
#define WPL_FAN_LOW       3

#define WPL_CMD_MODE      0x06

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return WPL_MODE_AUTO;
    case AC_MODE_COOL: return WPL_MODE_COOL;
    case AC_MODE_DRY:  return WPL_MODE_DRY;
    case AC_MODE_FAN:  return WPL_MODE_FAN;
    case AC_MODE_HEAT: return WPL_MODE_HEAT;
    default:           return WPL_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return WPL_FAN_AUTO;
    case AC_FAN_LOW:  return WPL_FAN_LOW;
    case AC_FAN_MED:  return WPL_FAN_MED;
    case AC_FAN_HIGH: return WPL_FAN_HIGH;
    default:          return WPL_FAN_AUTO;
    }
}

static uint8_t xor_range(const uint8_t *p, size_t n)
{
    uint8_t v = 0;
    for(size_t i = 0; i < n; i++) v ^= p[i];
    return v;
}

static void apply_state(uint8_t s[WPL_BYTES], const AcState *st)
{
    memset(s, 0, WPL_BYTES);
    /* DG11J1-04 / DG11J13A defaults: byte0=0x83, byte1=0x06, byte6=0x80. */
    s[0] = 0x83;
    s[1] = 0x06;
    s[6] = 0x80;

    uint8_t temp = st->temp_c;
    if(temp < WPL_MIN_TEMP) temp = WPL_MIN_TEMP;
    if(temp > WPL_MAX_TEMP) temp = WPL_MAX_TEMP;
    if(st->mode == AC_MODE_AUTO) temp = WPL_AUTO_TEMP;

    /* byte 2: Fan(2) Power(1) Sleep(1) :3 Swing1(1) */
    s[2] = (uint8_t)((map_fan(st->fan) & 0x03)
                   | (st->power ? (1 << 2) : 0)
                   | (st->swing ? (1 << 7) : 0));

    /* byte 3: Mode(3) :1 Temp(4) -- Temp = degrees - MinTemp. */
    s[3] = (uint8_t)((map_mode(st->mode) & 0x07)
                   | (((temp - WPL_MIN_TEMP) & 0x0F) << 4));

    /* byte 8: Swing2 mirrors Swing1 (bit 6). */
    s[8] = (uint8_t)(st->swing ? (1 << 6) : 0);

    /* byte 15: Cmd = Mode-button, gives a sane default request. */
    s[15] = WPL_CMD_MODE;

    /* Checksums: byte 13 = xor(state[2..12]); byte 20 = xor(state[14..19]). */
    s[13] = xor_range(s + 2, 11);
    s[20] = xor_range(s + 14, 6);
}

static esp_err_t whirlpool_encode(const AcState *st,
                                  uint16_t **out_t, size_t *out_n,
                                  uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[WPL_BYTES];
    apply_state(s, st);

    /* hdr(2) + 6*16 + footer(2) + 8*16 + footer(2) + 7*16 + footer(2) = 346. */
    const size_t cap = 400;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = WPL_HDR_MARK;
    buf[n++] = WPL_HDR_SPACE;

    /* Section 1: bytes 0..5 */
    for(int i = 0; i < 6; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         WPL_BIT_MARK, WPL_ONE_SPACE, WPL_ZERO_SPACE);
    }
    buf[n++] = WPL_BIT_MARK;
    buf[n++] = WPL_GAP;

    /* Section 2: bytes 6..13 */
    for(int i = 6; i < 14; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         WPL_BIT_MARK, WPL_ONE_SPACE, WPL_ZERO_SPACE);
    }
    buf[n++] = WPL_BIT_MARK;
    buf[n++] = WPL_GAP;

    /* Section 3: bytes 14..20 */
    for(int i = 14; i < WPL_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         WPL_BIT_MARK, WPL_ONE_SPACE, WPL_ZERO_SPACE);
    }
    buf[n++] = WPL_BIT_MARK;
    buf[n++] = WPL_END_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = WPL_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_whirlpool = {
    .name   = "Whirlpool AC",
    .encode = whirlpool_encode,
};
