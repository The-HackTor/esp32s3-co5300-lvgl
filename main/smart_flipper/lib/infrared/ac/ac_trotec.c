#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define TROT_BYTES         9
#define TROT_FREQ_HZ       36000

#define TROT_HDR_MARK      5952
#define TROT_HDR_SPACE     7364
#define TROT_BIT_MARK      592
#define TROT_ONE_SPACE     1560
#define TROT_ZERO_SPACE    592
#define TROT_GAP           6184
#define TROT_END_GAP       1500

#define TROT_INTRO1        0x12
#define TROT_INTRO2        0x34

#define TROT_MIN_TEMP      18
#define TROT_MAX_TEMP      32

#define TROT_MODE_AUTO     0
#define TROT_MODE_COOL     1
#define TROT_MODE_DRY      2
#define TROT_MODE_FAN      3

#define TROT_FAN_LOW       1
#define TROT_FAN_MED       2
#define TROT_FAN_HIGH      3

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return TROT_MODE_AUTO;
    case AC_MODE_COOL: return TROT_MODE_COOL;
    case AC_MODE_DRY:  return TROT_MODE_DRY;
    case AC_MODE_FAN:  return TROT_MODE_FAN;
    case AC_MODE_HEAT: return TROT_MODE_AUTO;   /* No Heat in Trotec. */
    default:           return TROT_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return TROT_FAN_MED;      /* No Auto -> default Med. */
    case AC_FAN_LOW:  return TROT_FAN_LOW;
    case AC_FAN_MED:  return TROT_FAN_MED;
    case AC_FAN_HIGH: return TROT_FAN_HIGH;
    default:          return TROT_FAN_MED;
    }
}

static void apply_state(uint8_t s[TROT_BYTES], const AcState *st)
{
    memset(s, 0, TROT_BYTES);
    s[0] = TROT_INTRO1;
    s[1] = TROT_INTRO2;

    uint8_t temp = st->temp_c;
    if(temp < TROT_MIN_TEMP) temp = TROT_MIN_TEMP;
    if(temp > TROT_MAX_TEMP) temp = TROT_MAX_TEMP;

    /* byte 2: Mode(2) :1 Power(1) Fan(2) :2 */
    s[2] = (uint8_t)((map_mode(st->mode) & 0x03)
                  | (st->power ? (1 << 3) : 0)
                  | ((map_fan(st->fan)  & 0x03) << 4));

    /* byte 3: Temp(4) :3 Sleep(1) -- Temp = degrees - MinTemp. */
    s[3] = (uint8_t)((temp - TROT_MIN_TEMP) & 0x0F);

    /* byte 8: checksum = sumBytes(state[0..7]) per upstream calcChecksum
     * (which is sumBytes(state+2, length-3) but checksum() sets _.Sum by
     * summing from 2.. anyway we compute the same via the .cpp:
     *   _.Sum = sumBytes(_.raw + 2, kTrotecStateLength - 3); */
    uint8_t sum = 0;
    for(int i = 2; i < TROT_BYTES - 1; i++) sum += s[i];
    s[8] = sum;
}

static esp_err_t trotec_encode(const AcState *st,
                               uint16_t **out_t, size_t *out_n,
                               uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[TROT_BYTES];
    apply_state(s, st);

    /* hdr(2) + 9*16 + footer(2) + end-gap(0; merged into final pair) ~ 150. */
    const size_t cap = 200;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = TROT_HDR_MARK;
    buf[n++] = TROT_HDR_SPACE;
    for(int i = 0; i < TROT_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         TROT_BIT_MARK, TROT_ONE_SPACE, TROT_ZERO_SPACE);
    }
    /* sendGeneric footer (mark + gap), then explicit end gap. */
    buf[n++] = TROT_BIT_MARK;
    buf[n++] = TROT_GAP;
    buf[n++] = TROT_BIT_MARK;
    buf[n++] = TROT_END_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = TROT_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_trotec = {
    .name   = "Trotec AC",
    .encode = trotec_encode,
};
