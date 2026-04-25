#include "mitsubishi.h"
#include "codec_match.h"

#include <string.h>

bool ir_mitsubishi_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_MITSUBISHI_BITS,
        0, 0,
        IR_MITSUBISHI_BIT_MARK, IR_MITSUBISHI_ONE_SPACE,
        IR_MITSUBISHI_BIT_MARK, IR_MITSUBISHI_ZERO_SPACE,
        IR_MITSUBISHI_BIT_MARK, IR_MITSUBISHI_MIN_GAP,
        true,
        30, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;
    out->bits  = IR_MITSUBISHI_BITS;
    out->value = value;
    return true;
}

bool ir_mitsubishi2_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI2;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_MITSUBISHI2_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_MITSUBISHI2_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    uint64_t value = 0;
    for(uint8_t i = 0; i < 2; i++) {
        uint64_t data = 0;
        const size_t used = ir_match_generic(
            timings, n_timings, cursor,
            IR_MITSUBISHI2_BITS / 2,
            0, 0,
            IR_MITSUBISHI2_BIT_MARK, IR_MITSUBISHI2_ONE_SPACE,
            IR_MITSUBISHI2_BIT_MARK, IR_MITSUBISHI2_ZERO_SPACE,
            IR_MITSUBISHI2_BIT_MARK,
            (i == 0) ? IR_MITSUBISHI2_HDR_SPACE : IR_MITSUBISHI2_MIN_GAP,
            (i % 2) != 0,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true,
            &data);
        if(used == 0) return false;
        cursor += used;
        value = (value << (IR_MITSUBISHI2_BITS / 2)) | data;
    }
    out->bits    = IR_MITSUBISHI2_BITS;
    out->value   = value;
    out->address = (uint32_t)((value >> (IR_MITSUBISHI2_BITS / 2))
                              & ((1U << (IR_MITSUBISHI2_BITS / 2)) - 1U));
    out->command = (uint32_t)(value & ((1U << (IR_MITSUBISHI2_BITS / 2)) - 1U));
    return true;
}

static bool decode_section_bytes(const uint16_t *timings, size_t n_timings,
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

bool ir_mitsubishi_ac_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI_AC;

    size_t cursor = 0;
    uint8_t saved[IR_MITSUBISHI_AC_STATE_BYTES];
    for(uint8_t r = 0; r < 2; r++) {
        if(!decode_section_bytes(
            timings, n_timings, &cursor,
            IR_MITSUBISHI_AC_HDR_MARK, IR_MITSUBISHI_AC_HDR_SPACE,
            IR_MITSUBISHI_AC_BIT_MARK, IR_MITSUBISHI_AC_ONE_SPACE,
            IR_MITSUBISHI_AC_BIT_MARK, IR_MITSUBISHI_AC_ZERO_SPACE,
            IR_MITSUBISHI_AC_RPT_MARK, IR_MITSUBISHI_AC_RPT_SPACE,
            r == 0,
            IR_DEFAULT_TOLERANCE + 5, 0,
            out->state, IR_MITSUBISHI_AC_STATE_BYTES)) return false;
        if(r == 0) {
            memcpy(saved, out->state, IR_MITSUBISHI_AC_STATE_BYTES);
        } else {
            if(memcmp(saved, out->state, IR_MITSUBISHI_AC_STATE_BYTES) != 0)
                return false;
        }
    }
    out->bits         = IR_MITSUBISHI_AC_BITS;
    out->state_nbytes = IR_MITSUBISHI_AC_STATE_BYTES;
    return true;
}

bool ir_mitsubishi112_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI112;

    if(n_timings == 0) return false;
    if(!ir_match_mark(timings[0], IR_MITSUBISHI112_HDR_MARK, 5, 0))
        return false;

    size_t cursor = 0;
    if(!decode_section_bytes(
        timings, n_timings, &cursor,
        IR_MITSUBISHI112_HDR_MARK, IR_MITSUBISHI112_HDR_SPACE,
        IR_MITSUBISHI112_BIT_MARK, IR_MITSUBISHI112_ONE_SPACE,
        IR_MITSUBISHI112_BIT_MARK, IR_MITSUBISHI112_ZERO_SPACE,
        IR_MITSUBISHI112_BIT_MARK, IR_MITSUBISHI112_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        out->state, IR_MITSUBISHI112_STATE_BYTES)) return false;

    out->bits         = IR_MITSUBISHI112_BITS;
    out->state_nbytes = IR_MITSUBISHI112_STATE_BYTES;
    return true;
}

bool ir_mitsubishi136_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI136;

    size_t cursor = 0;
    if(!decode_section_bytes(
        timings, n_timings, &cursor,
        IR_MITSUBISHI136_HDR_MARK, IR_MITSUBISHI136_HDR_SPACE,
        IR_MITSUBISHI136_BIT_MARK, IR_MITSUBISHI136_ONE_SPACE,
        IR_MITSUBISHI136_BIT_MARK, IR_MITSUBISHI136_ZERO_SPACE,
        IR_MITSUBISHI136_BIT_MARK, IR_MITSUBISHI136_GAP,
        true,
        IR_DEFAULT_TOLERANCE, 0,
        out->state, IR_MITSUBISHI136_STATE_BYTES)) return false;

    out->bits         = IR_MITSUBISHI136_BITS;
    out->state_nbytes = IR_MITSUBISHI136_STATE_BYTES;
    return true;
}
