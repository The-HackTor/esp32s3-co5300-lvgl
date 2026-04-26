#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#define MITSU_BYTES        18
#define MITSU_FREQ_HZ      38000

#define MITSU_HDR_MARK     3400
#define MITSU_HDR_SPACE    1750
#define MITSU_BIT_MARK     450
#define MITSU_ONE_SPACE    1300
#define MITSU_ZERO_SPACE   420
#define MITSU_RPT_MARK     440
#define MITSU_RPT_SPACE    15500

#define MITSU_MIN_TEMP     16
#define MITSU_MAX_TEMP     31

/* Reset signature, padded with zeros for bytes 11..17. */
static const uint8_t kReset[MITSU_BYTES] = {
    0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06,
    0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
};

/* Mode codes per IRremoteESP8266: Auto=4, Cool=3, Dry=2, Heat=1, Fan=7. */
static uint8_t map_mode(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return 0b100;
    case AC_MODE_COOL: return 0b011;
    case AC_MODE_DRY:  return 0b010;
    case AC_MODE_HEAT: return 0b001;
    case AC_MODE_FAN:  return 0b111;
    default:           return 0b100;
    }
}

/* Per upstream raw byte 8 patch by mode (mirrors setMode special bits). */
static uint8_t mode_byte8(AcMode m)
{
    switch(m) {
    case AC_MODE_AUTO: return 0b00110000;
    case AC_MODE_COOL: return 0b00110110;
    case AC_MODE_DRY:  return 0b00110010;
    case AC_MODE_HEAT: return 0b00110000;
    case AC_MODE_FAN:  return 0b00110111;
    default:           return 0b00110000;
    }
}

/* Fan: AcFan -> Mitsubishi (Fan:3, FanAuto:1) layout. Auto sets FanAuto=1
 * and Fan bits to 0; speeds use Fan=1..3 with FanAuto=0. */
static void map_fan_bits(AcFan f, uint8_t *fan_bits, uint8_t *fan_auto)
{
    switch(f) {
    case AC_FAN_AUTO: *fan_bits = 0; *fan_auto = 1; return;
    case AC_FAN_LOW:  *fan_bits = 1; *fan_auto = 0; return;
    case AC_FAN_MED:  *fan_bits = 2; *fan_auto = 0; return;
    case AC_FAN_HIGH: *fan_bits = 3; *fan_auto = 0; return;
    default:          *fan_bits = 0; *fan_auto = 1; return;
    }
}

static void apply_state(uint8_t s[MITSU_BYTES], const AcState *st)
{
    uint8_t temp = st->temp_c;
    if(temp < MITSU_MIN_TEMP) temp = MITSU_MIN_TEMP;
    if(temp > MITSU_MAX_TEMP) temp = MITSU_MAX_TEMP;

    /* Byte 5: Power at bit 5 (other bits keep their reset value 0x20). */
    s[5] = (uint8_t)((s[5] & ~(1 << 5)) | (st->power ? (1 << 5) : 0));

    /* Byte 6: Mode at bits 3..5. */
    s[6] = (uint8_t)((s[6] & ~(0x07 << 3)) | ((map_mode(st->mode) & 0x07) << 3));

    /* Byte 7: Temp at bits 0..3 (degrees - 16). HalfDegree at bit 4 cleared. */
    s[7] = (uint8_t)((s[7] & ~0x1F) | ((temp - MITSU_MIN_TEMP) & 0x0F));

    /* Byte 8: special per-mode raw value (upstream's setMode hack). */
    s[8] = mode_byte8(st->mode);

    /* Byte 9: Fan bits 0..2, Vane bits 3..5, FanAuto bit 7. Vane stays auto (0). */
    uint8_t fan_bits, fan_auto;
    map_fan_bits(st->fan, &fan_bits, &fan_auto);
    uint8_t vane = st->swing ? 0b111 : 0b000;   /* Auto-swing or fixed */
    s[9] = (uint8_t)((fan_bits & 0x07)
                   | ((vane & 0x07) << 3)
                   | (fan_auto ? (1 << 7) : 0));
}

static void emit_frame(uint16_t *buf, size_t cap, size_t *n,
                       const uint8_t s[MITSU_BYTES], bool last)
{
    if(*n + 2 > cap) return;
    buf[(*n)++] = MITSU_HDR_MARK;
    buf[(*n)++] = MITSU_HDR_SPACE;
    for(size_t i = 0; i < MITSU_BYTES; i++) {
        ac_push_byte_lsb(buf, cap, n, s[i],
                         MITSU_BIT_MARK, MITSU_ONE_SPACE, MITSU_ZERO_SPACE);
    }
    if(*n + 2 > cap) return;
    buf[(*n)++] = MITSU_RPT_MARK;
    buf[(*n)++] = last ? MITSU_RPT_SPACE : MITSU_RPT_SPACE;
    /* `last` reserved for future single-frame variants -- both branches
     * currently terminate identically, since the receiver expects the
     * inter-frame gap regardless. */
}

static esp_err_t mitsubishi_encode(const AcState *st,
                                   uint16_t **out_t, size_t *out_n,
                                   uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint8_t s[MITSU_BYTES];
    memcpy(s, kReset, MITSU_BYTES);
    apply_state(s, st);
    s[MITSU_BYTES - 1] = ac_sum_bytes(s, MITSU_BYTES - 1);

    /* Two frames * (header(2) + 18*16 + footer(2)) = 2*292 = 584. */
    const size_t cap = 700;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    emit_frame(buf, cap, &n, s, false);
    emit_frame(buf, cap, &n, s, true);

    *out_t    = buf;
    *out_n    = n;
    *out_freq = MITSU_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_mitsubishi = {
    .name   = "Mitsubishi AC",
    .encode = mitsubishi_encode,
};
