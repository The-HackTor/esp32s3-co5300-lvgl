#include "ac_brand.h"

#include <stdlib.h>

#define COOLIX_HDR_MARK    4692
#define COOLIX_HDR_SPACE   4416
#define COOLIX_BIT_MARK    552
#define COOLIX_ONE_SPACE   1656
#define COOLIX_ZERO_SPACE  552
#define COOLIX_MIN_GAP     5244
#define COOLIX_FREQ_HZ     38000
#define COOLIX_BITS        24

#define COOLIX_MIN_TEMP    17
#define COOLIX_MAX_TEMP    30
#define COOLIX_OFF_RAW     0xB27BE0u
#define COOLIX_DEFAULT_RAW 0xB21FC8u   /* On, 25C, Auto, Auto fan, no zone */

/* Coolix raw layout (LSB->MSB):
 *   bit 0   : unknown
 *   bit 1   : ZoneFollow1
 *   bits 2..3 : Mode (2 bits; high bit of upstream's 3-bit consts dropped)
 *   bits 4..7 : Temp (4-bit code from kCoolixTempMap)
 *   bits 8..12: SensorTemp (5 bits) -- 0x1F = ignore
 *   bits 13..15: Fan (3 bits)
 *   bits 16..18: unknown (3 bits)
 *   bit 19  : ZoneFollow2
 *   bits 20..23: signature 0xB
 */

static const uint8_t kTempMap[COOLIX_MAX_TEMP - COOLIX_MIN_TEMP + 1] = {
    0b0000, 0b0001, 0b0011, 0b0010, 0b0110, 0b0111, 0b0101, 0b0100,
    0b1100, 0b1101, 0b1001, 0b1000, 0b1010, 0b1011,
};

static uint8_t map_mode_2bit(AcMode m, uint8_t *out_fan_override)
{
    *out_fan_override = 0;   /* No override unless Fan mode. */
    switch(m) {
    case AC_MODE_COOL: return 0b00;
    case AC_MODE_DRY:  return 0b01;
    case AC_MODE_AUTO: return 0b10;
    case AC_MODE_HEAT: return 0b11;
    case AC_MODE_FAN:
        *out_fan_override = 0b1110;   /* kCoolixFanTempCode */
        return 0b00;
    default:           return 0b10;
    }
}

static uint8_t map_fan(AcFan f)
{
    switch(f) {
    case AC_FAN_AUTO: return 0b101;
    case AC_FAN_LOW:  return 0b100;
    case AC_FAN_MED:  return 0b010;
    case AC_FAN_HIGH: return 0b001;
    default:          return 0b101;
    }
}

static uint32_t build_state(const AcState *st)
{
    if(!st->power) return COOLIX_OFF_RAW;

    uint8_t temp = st->temp_c;
    if(temp < COOLIX_MIN_TEMP) temp = COOLIX_MIN_TEMP;
    if(temp > COOLIX_MAX_TEMP) temp = COOLIX_MAX_TEMP;

    uint8_t fan_override = 0;
    uint32_t mode = map_mode_2bit(st->mode, &fan_override);
    uint32_t fan  = fan_override ? fan_override : map_fan(st->fan);

    uint32_t raw = COOLIX_DEFAULT_RAW;
    raw &= ~(0x3u << 2);   raw |= (mode & 0x3) << 2;
    raw &= ~(0xFu << 4);   raw |= ((uint32_t)kTempMap[temp - COOLIX_MIN_TEMP] & 0xF) << 4;
    raw &= ~(0x1Fu << 8);  raw |= 0x1Fu << 8;     /* SensorTemp = ignore */
    raw &= ~(0x7u << 13);  raw |= (fan & 0x7) << 13;
    /* (void)swing; -- Coolix swing is a separate command; not folded into state. */
    return raw;
}

static void emit_byte_msb(uint16_t *buf, size_t cap, size_t *n, uint8_t b)
{
    for(int i = 7; i >= 0; i--) {
        if(*n + 2 > cap) return;
        buf[(*n)++] = COOLIX_BIT_MARK;
        buf[(*n)++] = ((b >> i) & 1) ? COOLIX_ONE_SPACE : COOLIX_ZERO_SPACE;
    }
}

static esp_err_t coolix_encode(const AcState *st,
                               uint16_t **out_t, size_t *out_n,
                               uint32_t *out_freq)
{
    if(!st || !out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    uint32_t raw = build_state(st);

    /* Header(2) + 6 bytes * 16 + footer(2). One frame; the AC scene re-emits
     * on each user action so a second-frame repeat is not strictly required. */
    const size_t cap = 2 + 6 * 16 + 2;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = COOLIX_HDR_MARK;
    buf[n++] = COOLIX_HDR_SPACE;

    /* 24 bits MSB-first, byte-by-byte, each byte followed by its bitwise
     * inverse, giving 48 transmitted bits per frame. */
    for(int byte = 2; byte >= 0; byte--) {
        uint8_t b = (raw >> (byte * 8)) & 0xFF;
        emit_byte_msb(buf, cap, &n, b);
        emit_byte_msb(buf, cap, &n, (uint8_t)(b ^ 0xFF));
    }

    buf[n++] = COOLIX_BIT_MARK;
    buf[n++] = COOLIX_MIN_GAP;

    *out_t    = buf;
    *out_n    = n;
    *out_freq = COOLIX_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_coolix = {
    .name   = "Coolix AC",
    .encode = coolix_encode,
};
