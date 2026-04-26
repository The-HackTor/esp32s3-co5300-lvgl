#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define KELV_BYTES         16
#define KELV_FREQ_HZ       38000

#define KELV_TICK          85
#define KELV_HDR_MARK      (106 * KELV_TICK)   /* 9010 */
#define KELV_HDR_SPACE     (53  * KELV_TICK)   /* 4505 */
#define KELV_BIT_MARK      (8   * KELV_TICK)   /* 680  */
#define KELV_ONE_SPACE     (18  * KELV_TICK)   /* 1530 */
#define KELV_ZERO_SPACE    (6   * KELV_TICK)   /* 510  */
#define KELV_GAP_SPACE     (235 * KELV_TICK)   /* 19975 */
/* GapSpace*2 = 39950us; hw_ir caps uint16_t at 32767us. */
#define KELV_BLOCK_GAP     30000

#define KELV_FOOTER        0b010
#define KELV_FOOTER_BITS   3

#define KELV_MIN_TEMP      16
#define KELV_MAX_TEMP      30
#define KELV_AUTO_TEMP     25
#define KELV_CHECKSUM_BASE 10

#define KELV_MODE_AUTO     0
#define KELV_MODE_COOL     1
#define KELV_MODE_DRY      2
#define KELV_MODE_FAN      3
#define KELV_MODE_HEAT     4

#define KELV_FAN_AUTO      0
#define KELV_FAN_LOW       1
#define KELV_FAN_MED       3
#define KELV_FAN_HIGH      5
#define KELV_BASIC_FAN_MAX 3

#define KELV_SWING_AUTO    0b0001

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return KELV_MODE_AUTO;
    case AC_MODE_COOL: return KELV_MODE_COOL;
    case AC_MODE_DRY:  return KELV_MODE_DRY;
    case AC_MODE_FAN:  return KELV_MODE_FAN;
    case AC_MODE_HEAT: return KELV_MODE_HEAT;
    default:           return KELV_MODE_AUTO;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return KELV_FAN_AUTO;
    case AC_FAN_LOW:  return KELV_FAN_LOW;
    case AC_FAN_MED:  return KELV_FAN_MED;
    case AC_FAN_HIGH: return KELV_FAN_HIGH;
    default:          return KELV_FAN_AUTO;
    }
}

/* Per-block sum: low nibbles of bytes 0..3 + high nibbles of bytes 4..6 + 10. */
static uint8_t kelv_block_checksum(const uint8_t *blk)
{
    uint8_t sum = KELV_CHECKSUM_BASE;
    for(int i = 0; i < 4; i++) sum += blk[i] & 0x0F;
    for(int i = 4; i < 7; i++) sum += (blk[i] >> 4) & 0x0F;
    return sum & 0x0F;
}

static void apply_state(uint8_t s[KELV_BYTES], const AcState *st)
{
    memset(s, 0, KELV_BYTES);
    /* stateReset constants: byte 3 high nibble + byte 11 = 0x70 marker. */
    s[3]  = 0x50;
    s[11] = 0x70;

    uint8_t mode = map_mode(st->mode);
    uint8_t fan  = map_fan(st->fan);

    uint8_t temp = st->temp_c;
    if(temp < KELV_MIN_TEMP) temp = KELV_MIN_TEMP;
    if(temp > KELV_MAX_TEMP) temp = KELV_MAX_TEMP;
    if(mode == KELV_MODE_AUTO || mode == KELV_MODE_DRY) temp = KELV_AUTO_TEMP;

    uint8_t basic_fan = (fan > KELV_BASIC_FAN_MAX) ? KELV_BASIC_FAN_MAX : fan;

    /* byte 0: Mode(3) Power(1) BasicFan(2) SwingAuto(1) :1 */
    s[0] = (uint8_t)((mode & 0x07)
                  | (st->power ? (1 << 3) : 0)
                  | ((basic_fan & 0x03) << 4)
                  | (st->swing ? (1 << 6) : 0));

    /* byte 1 low nibble: Temp - MinTemp. */
    s[1] = (uint8_t)((temp - KELV_MIN_TEMP) & 0x0F);

    /* byte 4: SwingV(4) SwingH(1) :3 -- Auto pattern when swing requested. */
    s[4] = st->swing ? KELV_SWING_AUTO : 0;

    /* byte 14 high 3 bits = Fan (advanced). */
    s[14] = (uint8_t)((fan & 0x07) << 4);

    /* fixup: bytes 8..10 mirror 0..2. */
    s[8]  = s[0];
    s[9]  = s[1];
    s[10] = s[2];

    /* Block checksums: high nibble of byte 7 / 15. */
    s[7]  = (uint8_t)((s[7]  & 0x0F) | ((kelv_block_checksum(s)     & 0x0F) << 4));
    s[15] = (uint8_t)((s[15] & 0x0F) | ((kelv_block_checksum(s + 8) & 0x0F) << 4));
}

static void emit_bytes(uint16_t *buf, size_t cap, size_t *n,
                       const uint8_t *bytes, size_t len)
{
    for(size_t i = 0; i < len; i++) {
        ac_push_byte_lsb(buf, cap, n, bytes[i],
                         KELV_BIT_MARK, KELV_ONE_SPACE, KELV_ZERO_SPACE);
    }
}

static void emit_footer(uint16_t *buf, size_t *n)
{
    for(int bit = 0; bit < KELV_FOOTER_BITS; bit++) {
        buf[(*n)++] = KELV_BIT_MARK;
        buf[(*n)++] = ((KELV_FOOTER >> bit) & 1) ? KELV_ONE_SPACE
                                                 : KELV_ZERO_SPACE;
    }
    buf[(*n)++] = KELV_BIT_MARK;
    buf[(*n)++] = KELV_GAP_SPACE;
}

static esp_err_t kelvinator_encode(const AcState *st,
                                   uint16_t **out_t, size_t *out_n,
                                   uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[KELV_BYTES];
    apply_state(s, st);

    /* 2 hdrs(4) + 16 bytes*16(256) + 2 mid-footers(6+2 each = 16) + 2 inter-block
     * gaps(4) + final gap(2) ~ 282. */
    const size_t cap = 320;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;

    /* Block 1: hdr + bytes 0..3 + footer + bytes 4..7 + 2x-gap. */
    buf[n++] = KELV_HDR_MARK;
    buf[n++] = KELV_HDR_SPACE;
    emit_bytes(buf, cap, &n, s, 4);
    emit_footer(buf, &n);
    emit_bytes(buf, cap, &n, s + 4, 4);
    buf[n++] = KELV_BIT_MARK;
    buf[n++] = KELV_BLOCK_GAP;

    /* Block 2: hdr + bytes 8..11 + footer + bytes 12..15 + final gap. */
    buf[n++] = KELV_HDR_MARK;
    buf[n++] = KELV_HDR_SPACE;
    emit_bytes(buf, cap, &n, s + 8, 4);
    emit_footer(buf, &n);
    emit_bytes(buf, cap, &n, s + 12, 4);
    buf[n++] = KELV_BIT_MARK;
    buf[n++] = KELV_BLOCK_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = KELV_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_kelvinator = {
    .name   = "Kelvinator AC",
    .encode = kelvinator_encode,
};
