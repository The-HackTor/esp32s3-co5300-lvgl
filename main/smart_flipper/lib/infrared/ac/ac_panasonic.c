#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define PANA_BYTES        27
#define PANA_SECTION1     8
#define PANA_FREQ_HZ      36700

#define PANA_HDR_MARK     3456
#define PANA_HDR_SPACE    1728
#define PANA_BIT_MARK     432
#define PANA_ONE_SPACE    1296
#define PANA_ZERO_SPACE   432
#define PANA_SECTION_GAP  10000
/* Upstream specifies 74736us, but hw_ir.c uint16_t timings cap at 32767us.
 * Receivers only need to see "no edge for >30ms" to end the frame. */
#define PANA_MESSAGE_GAP  30000

#define PANA_MIN_TEMP     16
#define PANA_MAX_TEMP     30
#define PANA_CHECK_INIT   0xF4
#define PANA_FAN_DELTA    3

static const uint8_t kReset[PANA_BYTES] = {
    0x02, 0x20, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x06, 0x02,
    0x20, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x0E, 0xE0, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00,
};

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return 0;
    case AC_MODE_DRY:  return 2;
    case AC_MODE_COOL: return 3;
    case AC_MODE_HEAT: return 4;
    case AC_MODE_FAN:  return 6;
    default:           return 0;
    }
}

static uint8_t map_fan(AcFan f)
{
    /* Stored value = upstream-speed + delta(3). Speeds: Low=1, Med=2,
     * High=3, Auto=7. */
    switch(f) {
    case AC_FAN_AUTO: return 7 + PANA_FAN_DELTA;   /* 0xA */
    case AC_FAN_LOW:  return 1 + PANA_FAN_DELTA;   /* 4   */
    case AC_FAN_MED:  return 2 + PANA_FAN_DELTA;   /* 5   */
    case AC_FAN_HIGH: return 3 + PANA_FAN_DELTA;   /* 6   */
    default:          return 7 + PANA_FAN_DELTA;
    }
}

static void apply_state(uint8_t s[PANA_BYTES], const AcState *st)
{
    uint8_t temp = st->temp_c;
    if(temp < PANA_MIN_TEMP) temp = PANA_MIN_TEMP;
    if(temp > PANA_MAX_TEMP) temp = PANA_MAX_TEMP;

    /* Byte 13: low nibble = Power(bit 0) + reserved bits, high nibble = Mode. */
    s[13] = (uint8_t)((s[13] & 0x0F & ~0x01) | (st->power ? 0x01 : 0x00));
    s[13] = (uint8_t)((s[13] & 0x0F) | ((map_mode(st->mode) & 0x0F) << 4));

    /* Byte 14: Temp at bits 1..5 (5 bits). */
    s[14] = (uint8_t)((s[14] & ~(0x1F << 1)) | ((temp & 0x1F) << 1));

    /* Byte 16: high nibble = Fan, low nibble = SwingV (0xF = Auto). */
    s[16] = (uint8_t)(((map_fan(st->fan) & 0x0F) << 4)
                    | ((st->swing ? 0xF : 0xF) & 0x0F));   /* keep Auto either way */

    /* Byte 26: checksum = sumBytes(0..25, init=0xF4). */
    uint8_t sum = PANA_CHECK_INIT;
    for(int i = 0; i < PANA_BYTES - 1; i++) sum += s[i];
    s[PANA_BYTES - 1] = sum;
}

static esp_err_t panasonic_encode(const AcState *st,
                                  uint16_t **out_t, size_t *out_n,
                                  uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[PANA_BYTES];
    memcpy(s, kReset, PANA_BYTES);
    apply_state(s, st);

    /* hdr(2) + sec1 (8*16) + footer(2 incl section gap) + hdr(2)
     * + sec2 (19*16) + footer(2). Worst case ~ 446. */
    const size_t cap = 500;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    /* Section 1 */
    buf[n++] = PANA_HDR_MARK;
    buf[n++] = PANA_HDR_SPACE;
    for(int i = 0; i < PANA_SECTION1; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         PANA_BIT_MARK, PANA_ONE_SPACE, PANA_ZERO_SPACE);
    }
    buf[n++] = PANA_BIT_MARK;
    buf[n++] = PANA_SECTION_GAP;

    /* Section 2 */
    buf[n++] = PANA_HDR_MARK;
    buf[n++] = PANA_HDR_SPACE;
    for(int i = PANA_SECTION1; i < PANA_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, &n, s[i],
                         PANA_BIT_MARK, PANA_ONE_SPACE, PANA_ZERO_SPACE);
    }
    buf[n++] = PANA_BIT_MARK;
    buf[n++] = PANA_MESSAGE_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = PANA_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_panasonic = {
    .name   = "Panasonic AC",
    .encode = panasonic_encode,
};
