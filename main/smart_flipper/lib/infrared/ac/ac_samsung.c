#include "ac_brand.h"

#include <stdlib.h>
#include <string.h>

#include "lib/infrared/codec_db/samsung.h"

#define SAMSUNG_AC_BYTES    14
#define SAMSUNG_AC_SECTION  7
#define SECTION_GAP_US      2886
#define DEFAULT_GAP_US      20000

static const uint8_t kReset[SAMSUNG_AC_BYTES] = {
    0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
    0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0,
};

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
    case AC_FAN_LOW:  return 2;
    case AC_FAN_MED:  return 4;
    case AC_FAN_HIGH: return 5;
    default:          return 0;
    }
}

static uint8_t count_bits(uint8_t b)
{
    uint8_t c = 0;
    for(int i = 0; i < 8; i++) c += (b >> i) & 1;
    return c;
}

static uint8_t section_checksum(const uint8_t *s)
{
    uint8_t sum = 0;
    sum += count_bits(s[0]);
    sum += count_bits(s[1] & 0x0F);
    sum += count_bits((s[2] >> 4) & 0x0F);
    for(int i = 3; i < 7; i++) sum += count_bits(s[i]);
    return sum ^ 0xFF;
}

static void apply_state(uint8_t state[SAMSUNG_AC_BYTES], const AcState *s)
{
    uint8_t temp = s->temp_c;
    if(temp < 16) temp = 16;
    if(temp > 30) temp = 30;

    state[6]  = (state[6] & ~(0x3 << 4)) | ((s->power ? 0x3 : 0x0) << 4);
    state[13] = (state[13] & ~(0x3 << 4)) | ((s->power ? 0x3 : 0x0) << 4);

    state[11] = (state[11] & 0x0F) | (((temp - 16) & 0x0F) << 4);

    state[12] = (state[12] & ~(0x7 << 4)) | ((map_mode(s->mode) & 0x7) << 4);
    state[12] = (state[12] & ~(0x7 << 1)) | ((map_fan(s->fan) & 0x7) << 1);

    /* Swing: byte 9 bits 4..6. 1=Vert+Horiz, 7=fixed (off). */
    state[9] = (state[9] & ~(0x7 << 4)) | (((s->swing ? 1 : 7) & 0x7) << 4);

    /* Section checksums (4-bit nibbles in bytes 1..2 and 8..9). */
    uint8_t s1 = section_checksum(state);
    uint8_t s2 = section_checksum(state + SAMSUNG_AC_SECTION);
    state[1] = (state[1] & 0xF0) | (s1 & 0x0F);
    state[2] = (state[2] & 0x0F) | (((s1 >> 4) & 0x0F) << 4);
    state[8] = (state[8] & 0xF0) | (s2 & 0x0F);
    state[9] = (state[9] & 0x0F) | (((s2 >> 4) & 0x0F) << 4);
}

static esp_err_t samsung_encode(const AcState *s,
                                uint16_t **out_timings, size_t *out_n,
                                uint32_t *out_freq_hz)
{
    if(!s || !out_timings || !out_n || !out_freq_hz) return ESP_ERR_INVALID_ARG;

    uint8_t state[SAMSUNG_AC_BYTES];
    memcpy(state, kReset, SAMSUNG_AC_BYTES);
    apply_state(state, s);

    /* HDR(2) + 2 sections * (header(2) + 56*2 bits + footer(2)) + trailing gap(1). */
    const size_t cap = 2 + 2 * (2 + 56 * 2 + 2) + 1;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    size_t n = 0;
    buf[n++] = IR_SAMSUNG_AC_HDR_MARK;
    buf[n++] = IR_SAMSUNG_AC_HDR_SPACE;

    for(int sec = 0; sec < 2; sec++) {
        buf[n++] = IR_SAMSUNG_AC_SECTION_MARK;
        buf[n++] = IR_SAMSUNG_AC_SECTION_SPACE;
        for(int i = 0; i < SAMSUNG_AC_SECTION; i++) {
            ac_push_byte_lsb(buf, cap, &n,
                             state[sec * SAMSUNG_AC_SECTION + i],
                             IR_SAMSUNG_AC_BIT_MARK,
                             IR_SAMSUNG_AC_ONE_SPACE,
                             IR_SAMSUNG_AC_ZERO_SPACE);
        }
        buf[n++] = IR_SAMSUNG_AC_BIT_MARK;
        buf[n++] = (sec == 0) ? SECTION_GAP_US : DEFAULT_GAP_US;
    }

    *out_timings = buf;
    *out_n       = n;
    *out_freq_hz = IR_SAMSUNG_AC_FREQ_HZ;
    return ESP_OK;
}

const AcBrand ac_brand_samsung = {
    .name   = "Samsung AC",
    .encode = samsung_encode,
};
