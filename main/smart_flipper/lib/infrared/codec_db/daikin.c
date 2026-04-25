#include "daikin.h"
#include "codec_match.h"

#include <string.h>

static bool decode_byte_section(const uint16_t *timings, size_t n_timings,
                                size_t *cursor,
                                uint16_t hdr_mark, uint32_t hdr_space,
                                uint16_t one_mark, uint32_t one_space,
                                uint16_t zero_mark, uint32_t zero_space,
                                uint16_t footer_mark, uint32_t footer_space,
                                bool at_least_footer,
                                uint8_t tolerance, int16_t excess,
                                uint8_t *out_state, uint16_t nbytes)
{
    size_t c = *cursor;
    if(hdr_mark) {
        if(c >= n_timings) return false;
        if(!ir_match_mark(timings[c], hdr_mark, tolerance, excess)) return false;
        c++;
    }
    if(hdr_space) {
        if(c >= n_timings) return false;
        if(!ir_match_space(timings[c], hdr_space, tolerance, excess)) return false;
        c++;
    }
    for(uint16_t b = 0; b < nbytes; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, c, 8,
            one_mark, one_space, zero_mark, zero_space,
            tolerance, excess, false, true);
        if(!r.success) return false;
        c += r.used;
        out_state[b] = (uint8_t)r.data;
    }
    if(footer_mark) {
        if(c >= n_timings) return false;
        if(!ir_match_mark(timings[c], footer_mark, tolerance, excess)) return false;
        c++;
    }
    if(footer_space) {
        const uint16_t measured = (c < n_timings) ? timings[c] : 0;
        if(at_least_footer) {
            if(!ir_match_at_least(measured, footer_space, tolerance, 0)) return false;
        } else {
            if(!ir_match_space(measured, footer_space, tolerance, excess)) return false;
        }
        if(c < n_timings) c++;
    }
    *cursor = c;
    return true;
}

bool ir_daikin64_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN64;

    size_t cursor = 0;
    for(uint8_t i = 0; i < 2; i++) {
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_DAIKIN64_LDR_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(cursor >= n_timings) return false;
        if(!ir_match_space(timings[cursor], IR_DAIKIN64_LDR_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
    }
    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, cursor,
        IR_DAIKIN64_BITS,
        IR_DAIKIN64_HDR_MARK, IR_DAIKIN64_HDR_SPACE,
        IR_DAIKIN64_BIT_MARK, IR_DAIKIN64_ONE_SPACE,
        IR_DAIKIN64_BIT_MARK, IR_DAIKIN64_ZERO_SPACE,
        IR_DAIKIN64_BIT_MARK, IR_DAIKIN64_GAP,
        false,
        IR_DEFAULT_TOLERANCE + 5, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;
    cursor += used;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_DAIKIN64_HDR_MARK,
                      IR_DEFAULT_TOLERANCE + 5, IR_MARK_EXCESS)) return false;

    out->bits  = IR_DAIKIN64_BITS;
    out->value = value;
    return true;
}

bool ir_daikin128_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN128;

    size_t cursor = 0;
    for(uint8_t i = 0; i < 2; i++) {
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_DAIKIN128_LDR_MARK,
                          IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(cursor >= n_timings) return false;
        if(!ir_match_space(timings[cursor], IR_DAIKIN128_LDR_SPACE,
                           IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
    }

    const uint16_t s1 = 8;
    const uint16_t s2 = IR_DAIKIN128_STATE_BYTES - s1;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN128_HDR_MARK, IR_DAIKIN128_HDR_SPACE,
        IR_DAIKIN128_BIT_MARK, IR_DAIKIN128_ONE_SPACE,
        IR_DAIKIN128_BIT_MARK, IR_DAIKIN128_ZERO_SPACE,
        IR_DAIKIN128_BIT_MARK, 0,
        false,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state, s1)) return false;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        0, 0,
        IR_DAIKIN128_BIT_MARK, IR_DAIKIN128_ONE_SPACE,
        IR_DAIKIN128_BIT_MARK, IR_DAIKIN128_ZERO_SPACE,
        IR_DAIKIN128_FOOTER_MARK, IR_DAIKIN128_GAP,
        true,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state + s1, s2)) return false;

    out->bits         = IR_DAIKIN128_BITS;
    out->state_nbytes = IR_DAIKIN128_STATE_BYTES;
    return true;
}

bool ir_daikin152_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN152;

    size_t cursor = 0;
    uint64_t leader = 0;
    const size_t lused = ir_match_generic(
        timings, n_timings, cursor,
        IR_DAIKIN152_LEADER_BITS,
        0, 0,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_ONE_SPACE,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_ZERO_SPACE,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_GAP,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &leader);
    if(lused == 0 || leader != 0) return false;
    cursor += lused;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN152_HDR_MARK, IR_DAIKIN152_HDR_SPACE,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_ONE_SPACE,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_ZERO_SPACE,
        IR_DAIKIN152_BIT_MARK, IR_DAIKIN152_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        out->state, IR_DAIKIN152_STATE_BYTES)) return false;

    out->bits         = IR_DAIKIN152_BITS;
    out->state_nbytes = IR_DAIKIN152_STATE_BYTES;
    return true;
}

bool ir_daikin160_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN160;

    size_t cursor = 0;
    const uint16_t s1 = 7;
    const uint16_t s2 = IR_DAIKIN160_STATE_BYTES - s1;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN160_HDR_MARK, IR_DAIKIN160_HDR_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_ONE_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_ZERO_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_GAP,
        false,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state, s1)) return false;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN160_HDR_MARK, IR_DAIKIN160_HDR_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_ONE_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_ZERO_SPACE,
        IR_DAIKIN160_BIT_MARK, IR_DAIKIN160_GAP,
        true,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state + s1, s2)) return false;

    out->bits         = IR_DAIKIN160_BITS;
    out->state_nbytes = IR_DAIKIN160_STATE_BYTES;
    return true;
}

bool ir_daikin176_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN176;

    size_t cursor = 0;
    const uint16_t s1 = 7;
    const uint16_t s2 = IR_DAIKIN176_STATE_BYTES - s1;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN176_HDR_MARK, IR_DAIKIN176_HDR_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_ONE_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_ZERO_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_GAP,
        false,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state, s1)) return false;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN176_HDR_MARK, IR_DAIKIN176_HDR_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_ONE_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_ZERO_SPACE,
        IR_DAIKIN176_BIT_MARK, IR_DAIKIN176_GAP,
        true,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state + s1, s2)) return false;

    out->bits         = IR_DAIKIN176_BITS;
    out->state_nbytes = IR_DAIKIN176_STATE_BYTES;
    return true;
}

bool ir_daikin200_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN200;

    size_t cursor = 0;
    const uint16_t s1 = 7;
    const uint16_t s2 = IR_DAIKIN200_STATE_BYTES - s1;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN200_HDR_MARK, IR_DAIKIN200_HDR_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_ONE_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_ZERO_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_GAP,
        false,
        IR_DAIKIN_TOLERANCE, 0,
        out->state, s1)) return false;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN200_HDR_MARK, IR_DAIKIN200_HDR_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_ONE_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_ZERO_SPACE,
        IR_DAIKIN200_BIT_MARK, IR_DAIKIN200_GAP,
        true,
        IR_DAIKIN_TOLERANCE, 0,
        out->state + s1, s2)) return false;

    out->bits         = IR_DAIKIN200_BITS;
    out->state_nbytes = IR_DAIKIN200_STATE_BYTES;
    return true;
}

bool ir_daikin216_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_DAIKIN216;

    size_t cursor = 0;
    const uint16_t s1 = 8;
    const uint16_t s2 = IR_DAIKIN216_STATE_BYTES - s1;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN216_HDR_MARK, IR_DAIKIN216_HDR_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_ONE_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_ZERO_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_GAP,
        false,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state, s1)) return false;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_DAIKIN216_HDR_MARK, IR_DAIKIN216_HDR_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_ONE_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_ZERO_SPACE,
        IR_DAIKIN216_BIT_MARK, IR_DAIKIN216_GAP,
        true,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        out->state + s1, s2)) return false;

    out->bits         = IR_DAIKIN216_BITS;
    out->state_nbytes = IR_DAIKIN216_STATE_BYTES;
    return true;
}

bool ir_daikin_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    size_t cursor = 0;
    uint64_t leader = 0;
    const size_t lused = ir_match_generic(
        timings, n_timings, cursor,
        IR_DAIKIN_HEADER_LENGTH,
        0, 0,
        IR_DAIKIN_BIT_MARK, IR_DAIKIN_ONE_SPACE,
        IR_DAIKIN_BIT_MARK, IR_DAIKIN_ZERO_SPACE,
        IR_DAIKIN_BIT_MARK, IR_DAIKIN_ZERO_SPACE,
        false,
        IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
        false,
        &leader);
    if(lused == 0 || leader != 0) return false;
    cursor += lused;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_DAIKIN_BIT_MARK,
                      IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor++],
                       IR_DAIKIN_ZERO_SPACE + IR_DAIKIN_GAP,
                       IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;
    const uint16_t section_len[3] = {
        IR_DAIKIN_SECTION1_LEN, IR_DAIKIN_SECTION2_LEN, IR_DAIKIN_SECTION3_LEN
    };
    uint16_t pos = 0;
    for(uint8_t s = 0; s < 3; s++) {
        const bool last = (s == 2);
        if(!decode_byte_section(
            timings, n_timings, &cursor,
            IR_DAIKIN_HDR_MARK, IR_DAIKIN_HDR_SPACE,
            IR_DAIKIN_BIT_MARK, IR_DAIKIN_ONE_SPACE,
            IR_DAIKIN_BIT_MARK, IR_DAIKIN_ZERO_SPACE,
            IR_DAIKIN_BIT_MARK, IR_DAIKIN_ZERO_SPACE + IR_DAIKIN_GAP,
            last,
            IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
            out->state + pos, section_len[s])) return false;
        pos += section_len[s];
    }

    out->id           = IR_CODEC_DAIKIN;
    out->bits         = IR_DAIKIN_BITS;
    out->state_nbytes = IR_DAIKIN_STATE_BYTES;
    return true;
}

bool ir_daikin2_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_DAIKIN2_LEADER_MARK,
                      IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor++], IR_DAIKIN2_LEADER_SPACE,
                       IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS)) return false;

    const uint16_t section_len[2] = {
        IR_DAIKIN2_SECTION1_LEN, IR_DAIKIN2_SECTION2_LEN
    };
    uint16_t pos = 0;
    for(uint8_t s = 0; s < 2; s++) {
        const bool last = (s == 1);
        if(!decode_byte_section(
            timings, n_timings, &cursor,
            IR_DAIKIN2_HDR_MARK, IR_DAIKIN2_HDR_SPACE,
            IR_DAIKIN2_BIT_MARK, IR_DAIKIN2_ONE_SPACE,
            IR_DAIKIN2_BIT_MARK, IR_DAIKIN2_ZERO_SPACE,
            IR_DAIKIN2_BIT_MARK, IR_DAIKIN2_GAP,
            last,
            IR_DAIKIN_TOLERANCE, IR_MARK_EXCESS,
            out->state + pos, section_len[s])) return false;
        pos += section_len[s];
    }

    out->id           = IR_CODEC_DAIKIN2;
    out->bits         = IR_DAIKIN2_BITS;
    out->state_nbytes = IR_DAIKIN2_STATE_BYTES;
    return true;
}

bool ir_daikin312_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    size_t cursor = 0;
    uint64_t leader = 0;
    const size_t lused = ir_match_generic(
        timings, n_timings, cursor,
        IR_DAIKIN312_HEADER_LENGTH,
        0, 0,
        IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_ONE_SPACE,
        IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_ZERO_SPACE,
        IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_HDR_GAP,
        false,
        IR_DAIKIN_TOLERANCE, 0,
        false,
        &leader);
    if(lused == 0 || leader != 0) return false;
    cursor += lused;

    const uint16_t section_len[2] = {
        IR_DAIKIN312_SECTION1_LEN, IR_DAIKIN312_SECTION2_LEN
    };
    uint16_t pos = 0;
    for(uint8_t s = 0; s < 2; s++) {
        const bool last = (s == 1);
        if(!decode_byte_section(
            timings, n_timings, &cursor,
            IR_DAIKIN312_HDR_MARK, IR_DAIKIN312_HDR_SPACE,
            IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_ONE_SPACE,
            IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_ZERO_SPACE,
            IR_DAIKIN312_BIT_MARK, IR_DAIKIN312_SECTION_GAP,
            last,
            IR_DAIKIN_TOLERANCE, 0,
            out->state + pos, section_len[s])) return false;
        pos += section_len[s];
    }

    out->id           = IR_CODEC_DAIKIN312;
    out->bits         = IR_DAIKIN312_BITS;
    out->state_nbytes = IR_DAIKIN312_STATE_BYTES;
    return true;
}
