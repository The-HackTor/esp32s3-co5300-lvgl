#include "xmp.h"
#include "codec_match.h"

#include <string.h>

static bool xmp_match_mark(uint16_t measured)
{
    return ir_match(measured, IR_XMP_MARK + IR_MARK_EXCESS, 0, IR_XMP_SPACE_STEP / 2);
}

static bool xmp_match_space(uint16_t measured, uint32_t desired)
{
    if(desired < (uint32_t)IR_MARK_EXCESS) desired = 0;
    else desired -= (uint32_t)IR_MARK_EXCESS;
    return ir_match(measured, desired, 0, IR_XMP_SPACE_STEP / 2);
}

static uint8_t xmp_section_checksum(uint64_t section, uint16_t section_bits)
{
    if(section_bits < 2U * IR_XMP_WORD_SIZE) return 0;
    return (uint8_t)((section >> (section_bits - 2U * IR_XMP_WORD_SIZE)) & 0xFU);
}

static uint8_t xmp_calc_checksum(uint64_t section, uint16_t section_bits)
{
    uint16_t nibbles = section_bits / IR_XMP_WORD_SIZE;
    uint8_t sum = 0xF;
    for(uint16_t i = 0; i < nibbles; i++) {
        sum += (uint8_t)((section >> (i * IR_XMP_WORD_SIZE)) & 0xFU);
    }
    sum &= 0xF;
    uint8_t stored = xmp_section_checksum(section, section_bits);
    return (uint8_t)(0xF & ~(sum - stored));
}

static bool xmp_is_repeat(uint64_t data, uint16_t nbits)
{
    if(nbits < 3U * IR_XMP_WORD_SIZE) return false;
    uint16_t offset = (nbits / IR_XMP_SECTIONS) - (3U * IR_XMP_WORD_SIZE);
    uint8_t nibble = (uint8_t)((data >> offset) & 0xFU);
    return nibble == IR_XMP_REPEAT_CODE || nibble == IR_XMP_REPEAT_CODE_ALT;
}

bool ir_xmp_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    const uint16_t nbits = IR_XMP_BITS;
    const uint16_t section_bits = nbits / IR_XMP_SECTIONS;
    size_t offset = 0;
    uint64_t data = 0;

    for(uint8_t section = 1; section <= IR_XMP_SECTIONS; section++) {
        for(uint16_t bits_so_far = 0; bits_so_far < section_bits; bits_so_far += IR_XMP_WORD_SIZE) {
            if(offset >= n_timings) return false;
            if(!xmp_match_mark(timings[offset++])) return false;
            if(offset >= n_timings) return false;

            uint8_t value = 0;
            bool found = false;
            for(; value <= IR_XMP_MAX_WORD_VALUE; value++) {
                uint32_t expected = IR_XMP_BASE_SPACE + (uint32_t)value * IR_XMP_SPACE_STEP;
                if(xmp_match_space(timings[offset], expected)) {
                    found = true;
                    break;
                }
            }
            if(!found) return false;
            data = (data << IR_XMP_WORD_SIZE) | value;
            offset++;
        }

        if(offset >= n_timings) return false;
        if(!xmp_match_mark(timings[offset++])) return false;

        if(section < IR_XMP_SECTIONS) {
            if(offset >= n_timings) return false;
            if(!ir_match_space(timings[offset++], IR_XMP_FOOTER_SPACE,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        } else if(offset < n_timings) {
            if(!ir_match_at_least(timings[offset++], IR_XMP_FOOTER_SPACE,
                                  IR_DEFAULT_TOLERANCE, 0)) return false;
        }
    }

    uint64_t cs_data = data;
    for(uint16_t s = 0; s < IR_XMP_SECTIONS; s++) {
        uint64_t section_val = cs_data & ((1ULL << section_bits) - 1ULL);
        if(xmp_section_checksum(section_val, section_bits) !=
           xmp_calc_checksum(section_val, section_bits)) return false;
        cs_data >>= section_bits;
    }

    memset(out, 0, sizeof(*out));
    out->id     = IR_CODEC_XMP;
    out->bits   = nbits;
    out->value  = data;
    out->repeat = xmp_is_repeat(data, nbits);
    return true;
}
