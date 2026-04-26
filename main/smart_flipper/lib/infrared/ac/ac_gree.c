#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define GREE_BYTES         8
#define GREE_BLOCK_LEN     4
#define GREE_FREQ_HZ       38000

#define GREE_HDR_MARK      9000
#define GREE_HDR_SPACE     4500
#define GREE_BIT_MARK      620
#define GREE_ONE_SPACE     1600
#define GREE_ZERO_SPACE    540
#define GREE_MSG_SPACE     19980

/* Mid-message footer is the 3-bit value 0b010, sent LSB-first. */
#define GREE_BLOCK_FOOTER     0b010
#define GREE_BLOCK_FOOTER_BITS 3

#define GREE_MIN_TEMP      16
#define GREE_MAX_TEMP      30
#define GREE_CHECKSUM_BASE 10

/* Gree state byte layout per IRremoteESP8266:
 *   byte 0: Mode(3) Power(1) Fan(2) SwingAuto(1) Sleep(1)
 *   byte 1: Temp(4) TimerHalfHr(1) TimerTensHr(2) TimerEnabled(1)
 *   byte 2: TimerHours(4) Turbo(1) Light(1) ModelA(1) Xfan(1)
 *   byte 3: 0(2) TempExtraDegreeF(1) UseFahrenheit(1) unknown1=0b0101(4)
 *   byte 4: SwingV(4) SwingH(3) 0(1)
 *   byte 5: DisplayTemp(2) IFeel(1) unknown2=0b100(3) WiFi(1) 0(1)
 *   byte 6: 0(8)
 *   byte 7: 0(2) Econo(1) 0(1) Sum(4)  -- Sum in high nibble
 */

static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return 0;
    case AC_MODE_COOL: return 1;
    case AC_MODE_DRY:  return 2;
    case AC_MODE_FAN:  return 3;
    case AC_MODE_HEAT: return 4;
    default:           return 0;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return 0;
    case AC_FAN_LOW:  return 1;
    case AC_FAN_MED:  return 2;
    case AC_FAN_HIGH: return 3;
    default:          return 0;
    }
}

/* Block checksum: sum lower nibbles of bytes 0..3, plus upper nibbles of
 * bytes 4..6, plus a magic constant 10. Result is the high nibble of byte 7. */
static uint8_t gree_block_checksum(const uint8_t s[GREE_BYTES])
{
    uint8_t sum = GREE_CHECKSUM_BASE;
    for(int i = 0; i < 4; i++)            sum += (s[i]   & 0x0F);
    for(int i = 4; i < GREE_BYTES - 1; i++) sum += (s[i] >> 4) & 0x0F;
    return sum & 0x0F;
}

static void apply_state(uint8_t s[GREE_BYTES], const AcState *st)
{
    memset(s, 0, GREE_BYTES);
    /* Constant fields per stateReset. */
    s[1] = 9;          /* Temp = 9 -> 25C (unknown1's effect is overwritten below) */
    s[2] = 0x20;       /* Light bit (byte 2 bit 5) */
    s[3] = 0x50;       /* unknown1 = 0b0101 */
    s[5] = 0x20;       /* unknown2 = 0b100 (byte 5 bits 3..5) */

    uint8_t temp = st->temp_c;
    if(temp < GREE_MIN_TEMP) temp = GREE_MIN_TEMP;
    if(temp > GREE_MAX_TEMP) temp = GREE_MAX_TEMP;

    /* byte 0: Mode(3) Power(1) Fan(2) SwingAuto(1) Sleep(1) */
    s[0] = (uint8_t)((map_mode(st->mode) & 0x07)
                   | (st->power  ? (1 << 3) : 0)
                   | ((map_fan(st->fan) & 0x03) << 4)
                   | (st->swing  ? (1 << 6) : 0));

    /* byte 1 low nibble: Temp = degrees - 16. Keeps timer bits zero. */
    s[1] = (uint8_t)((temp - GREE_MIN_TEMP) & 0x0F);

    /* Sum -> byte 7 high nibble. */
    s[7] = (uint8_t)((gree_block_checksum(s) & 0x0F) << 4);
}

static void emit_block(uint16_t *buf, size_t cap, size_t *n,
                       const uint8_t *bytes, size_t len)
{
    for(size_t i = 0; i < len; i++) {
        ac_push_byte_lsb(buf, cap, n, bytes[i],
                         GREE_BIT_MARK, GREE_ONE_SPACE, GREE_ZERO_SPACE);
    }
}

static esp_err_t gree_encode(const AcState *st,
                             uint16_t **out_t, size_t *out_n,
                             uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[GREE_BYTES];
    apply_state(s, st);

    /* Header(2) + 4*16 (block 1) + 6 (mid-footer 3 bits + mark + msg-space)
     * + 4*16 (block 2) + 2 (final mark + gap). */
    const size_t cap = 200;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = GREE_HDR_MARK;
    buf[n++] = GREE_HDR_SPACE;

    /* Block 1: bytes 0..3 */
    emit_block(buf, cap, &n, s, GREE_BLOCK_LEN);

    /* Mid-message footer: 3 LSB-first bits (0b010), then mark + GREE_MSG_SPACE. */
    for(int bit = 0; bit < GREE_BLOCK_FOOTER_BITS; bit++) {
        buf[n++] = GREE_BIT_MARK;
        buf[n++] = ((GREE_BLOCK_FOOTER >> bit) & 1) ? GREE_ONE_SPACE
                                                    : GREE_ZERO_SPACE;
    }
    buf[n++] = GREE_BIT_MARK;
    buf[n++] = GREE_MSG_SPACE;

    /* Block 2: bytes 4..7 */
    emit_block(buf, cap, &n, s + GREE_BLOCK_LEN, GREE_BYTES - GREE_BLOCK_LEN);

    buf[n++] = GREE_BIT_MARK;
    buf[n++] = GREE_MSG_SPACE;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = GREE_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_gree = {
    .name   = "Gree AC",
    .encode = gree_encode,
};
