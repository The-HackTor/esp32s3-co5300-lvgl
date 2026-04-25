#include "carrier.h"
#include "codec_match.h"

#include <string.h>

bool ir_carrier_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_CARRIER_AC;

    size_t cursor = 0;
    uint64_t data = 0;
    uint64_t prev = 0;
    for(uint8_t i = 0; i < 3; i++) {
        prev = data;
        const size_t used = ir_match_generic(
            timings, n_timings, cursor,
            IR_CARRIER_AC_BITS,
            IR_CARRIER_AC_HDR_MARK, IR_CARRIER_AC_HDR_SPACE,
            IR_CARRIER_AC_BIT_MARK, IR_CARRIER_AC_ONE_SPACE,
            IR_CARRIER_AC_BIT_MARK, IR_CARRIER_AC_ZERO_SPACE,
            IR_CARRIER_AC_BIT_MARK, IR_CARRIER_AC_GAP,
            true,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true,
            &data);
        if(used == 0) return false;
        cursor += used;
        if(i > 0) {
            const uint64_t mask = (IR_CARRIER_AC_BITS >= 64)
                                  ? ~(uint64_t)0
                                  : ((1ULL << IR_CARRIER_AC_BITS) - 1ULL);
            if(prev != ((~data) & mask)) return false;
        }
    }
    out->bits    = IR_CARRIER_AC_BITS;
    out->value   = data;
    out->address = (uint32_t)(data >> 16);
    out->command = (uint32_t)(data & 0xFFFF);
    return true;
}

bool ir_carrier_ac40_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_CARRIER_AC40;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_CARRIER_AC40_BITS,
        IR_CARRIER_AC40_HDR_MARK, IR_CARRIER_AC40_HDR_SPACE,
        IR_CARRIER_AC40_BIT_MARK, IR_CARRIER_AC40_ONE_SPACE,
        IR_CARRIER_AC40_BIT_MARK, IR_CARRIER_AC40_ZERO_SPACE,
        IR_CARRIER_AC40_BIT_MARK, IR_CARRIER_AC40_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    out->bits  = IR_CARRIER_AC40_BITS;
    out->value = value;
    return true;
}

bool ir_carrier_ac64_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_CARRIER_AC64;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_CARRIER_AC64_BITS,
        IR_CARRIER_AC64_HDR_MARK, IR_CARRIER_AC64_HDR_SPACE,
        IR_CARRIER_AC64_BIT_MARK, IR_CARRIER_AC64_ONE_SPACE,
        IR_CARRIER_AC64_BIT_MARK, IR_CARRIER_AC64_ZERO_SPACE,
        IR_CARRIER_AC64_BIT_MARK, IR_CARRIER_AC64_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    out->bits  = IR_CARRIER_AC64_BITS;
    out->value = value;
    return true;
}

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

bool ir_carrier_ac128_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_CARRIER_AC128;

    const uint16_t section_bytes = (IR_CARRIER_AC128_BITS / 2) / 8;
    size_t cursor = 0;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_CARRIER_AC128_HDR_MARK, IR_CARRIER_AC128_HDR_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_ONE_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_ZERO_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_SECTION_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        out->state, section_bytes)) return false;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_CARRIER_AC128_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_CARRIER_AC128_INTER_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_CARRIER_AC128_HDR2_MARK, IR_CARRIER_AC128_HDR2_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_ONE_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_ZERO_SPACE,
        IR_CARRIER_AC128_BIT_MARK, IR_CARRIER_AC128_SECTION_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        out->state + section_bytes, section_bytes)) return false;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_CARRIER_AC128_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    out->bits         = IR_CARRIER_AC128_BITS;
    out->state_nbytes = section_bytes * 2;
    return true;
}

bool ir_carrier_ac84_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_CARRIER_AC84;

    const uint8_t tol = IR_DEFAULT_TOLERANCE + IR_CARRIER_AC84_EXTRA_TOLERANCE;

    uint64_t extra = 0;
    const size_t used_hdr = ir_match_const_bit_time(
        timings, n_timings, 0,
        IR_CARRIER_AC84_EXTRA_BITS,
        IR_CARRIER_AC84_HDR_MARK, IR_CARRIER_AC84_HDR_SPACE,
        IR_CARRIER_AC84_ZERO, IR_CARRIER_AC84_ONE,
        0, 0,
        false,
        tol, IR_MARK_EXCESS,
        false,
        &extra);
    if(used_hdr == 0) return false;

    out->state[0] = (uint8_t)(extra & 0x0F);
    size_t cursor = used_hdr;

    const uint16_t data_bytes = (IR_CARRIER_AC84_BITS - IR_CARRIER_AC84_EXTRA_BITS) / 8;
    if(IR_CARRIER_AC84_STATE_LEN > IR_DECODE_STATE_MAX) return false;

    for(uint16_t b = 0; b < data_bytes; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_CARRIER_AC84_ZERO, IR_CARRIER_AC84_ONE,
            IR_CARRIER_AC84_ONE, IR_CARRIER_AC84_ZERO,
            tol, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        out->state[1 + b] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_CARRIER_AC84_ZERO, tol, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_CARRIER_AC84_GAP, tol, 0)) return false;
    }

    out->bits         = IR_CARRIER_AC84_BITS;
    out->state_nbytes = IR_CARRIER_AC84_STATE_LEN;
    return true;
}
