#include "samsung.h"
#include "codec_match.h"

#include <string.h>

bool ir_samsung36_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t high = 0;
    size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_SAMSUNG36_BITS_FIRST,
        IR_SAMSUNG36_HDR_MARK, IR_SAMSUNG36_HDR_SPACE,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_ONE_SPACE,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_ZERO_SPACE,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_HDR_SPACE,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &high);
    if(used == 0) return false;

    uint64_t low = 0;
    const uint16_t low_bits = IR_SAMSUNG36_BITS - IR_SAMSUNG36_BITS_FIRST;
    const size_t used2 = ir_match_generic(
        timings, n_timings, used,
        low_bits,
        0, 0,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_ONE_SPACE,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_ZERO_SPACE,
        IR_SAMSUNG36_BIT_MARK, IR_SAMSUNG36_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &low);
    if(used2 == 0) return false;

    const uint64_t value = (high << low_bits) | low;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_SAMSUNG36;
    out->bits    = IR_SAMSUNG36_BITS;
    out->value   = value;
    out->command = (uint32_t)(value & ((1ULL << low_bits) - 1ULL));
    out->address = (uint32_t)(value >> low_bits);
    return true;
}

bool ir_samsung_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  IR_SAMSUNG_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_SAMSUNG_AC_STATE_BYTES];
    const uint16_t total_bytes = IR_SAMSUNG_AC_STATE_BYTES;
    const uint16_t section_bytes = IR_SAMSUNG_AC_SECTION_LENGTH;

    for(uint16_t pos = 0; pos + section_bytes <= total_bytes; pos += section_bytes) {
        if(cursor + 1 >= n_timings) return false;
        if(!ir_match_mark(timings[cursor++],  IR_SAMSUNG_AC_SECTION_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_SECTION_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

        for(uint16_t i = 0; i < section_bytes; i++) {
            ir_match_result_t r = ir_match_data(
                timings, n_timings, cursor, 8,
                IR_SAMSUNG_AC_BIT_MARK, IR_SAMSUNG_AC_ONE_SPACE,
                IR_SAMSUNG_AC_BIT_MARK, IR_SAMSUNG_AC_ZERO_SPACE,
                IR_DEFAULT_TOLERANCE, 0,
                false, true);
            if(!r.success) return false;
            state[pos + i] = (uint8_t)r.data;
            cursor += r.used;
        }

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor++], IR_SAMSUNG_AC_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

        const bool last_section = (pos + section_bytes >= total_bytes);
        if(!last_section) {
            if(cursor >= n_timings) return false;
            if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_SECTION_GAP,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        }
    }

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SAMSUNG_AC;
    out->bits         = IR_SAMSUNG_AC_BITS;
    memcpy(out->state, state, IR_SAMSUNG_AC_STATE_BYTES);
    out->state_nbytes = IR_SAMSUNG_AC_STATE_BYTES;
    return true;
}

bool ir_samsung_ac256_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  IR_SAMSUNG_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_SAMSUNG_AC256_STATE_BYTES];
    const uint16_t total_bytes = IR_SAMSUNG_AC256_STATE_BYTES;
    const uint16_t section_bytes = IR_SAMSUNG_AC_SECTION_LENGTH;

    for(uint16_t pos = 0; pos + section_bytes <= total_bytes; pos += section_bytes) {
        if(cursor + 1 >= n_timings) return false;
        if(!ir_match_mark(timings[cursor++],  IR_SAMSUNG_AC_SECTION_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_SECTION_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

        for(uint16_t i = 0; i < section_bytes; i++) {
            ir_match_result_t r = ir_match_data(
                timings, n_timings, cursor, 8,
                IR_SAMSUNG_AC_BIT_MARK, IR_SAMSUNG_AC_ONE_SPACE,
                IR_SAMSUNG_AC_BIT_MARK, IR_SAMSUNG_AC_ZERO_SPACE,
                IR_DEFAULT_TOLERANCE, 0,
                false, true);
            if(!r.success) return false;
            state[pos + i] = (uint8_t)r.data;
            cursor += r.used;
        }

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor++], IR_SAMSUNG_AC_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

        const bool last_section = (pos + section_bytes >= total_bytes);
        if(!last_section) {
            if(cursor >= n_timings) return false;
            if(!ir_match_space(timings[cursor++], IR_SAMSUNG_AC_SECTION_GAP,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        }
    }

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SAMSUNG_AC_EXTENDED;
    out->bits         = IR_SAMSUNG_AC256_BITS;
    memcpy(out->state, state, IR_SAMSUNG_AC256_STATE_BYTES);
    out->state_nbytes = IR_SAMSUNG_AC256_STATE_BYTES;
    return true;
}
