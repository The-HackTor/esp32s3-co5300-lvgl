#include "trotec.h"
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

bool ir_trotec_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_TROTEC;

    size_t cursor = 0;
    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_TROTEC_HDR_MARK, IR_TROTEC_HDR_SPACE,
        IR_TROTEC_BIT_MARK, IR_TROTEC_ONE_SPACE,
        IR_TROTEC_BIT_MARK, IR_TROTEC_ZERO_SPACE,
        IR_TROTEC_BIT_MARK, IR_TROTEC_GAP,
        false,
        IR_DEFAULT_TOLERANCE, 0,
        out->state, IR_TROTEC_STATE_BYTES)) return false;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TROTEC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_TROTEC_GAP_END,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }

    out->bits         = IR_TROTEC_BITS;
    out->state_nbytes = IR_TROTEC_STATE_BYTES;
    return true;
}

bool ir_trotec3550_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_TROTEC_3550;

    size_t cursor = 0;
    if(!decode_byte_section(
        timings, n_timings, &cursor,
        IR_TROTEC3550_HDR_MARK, IR_TROTEC3550_HDR_SPACE,
        IR_TROTEC3550_BIT_MARK, IR_TROTEC3550_ONE_SPACE,
        IR_TROTEC3550_BIT_MARK, IR_TROTEC3550_ZERO_SPACE,
        IR_TROTEC3550_BIT_MARK, IR_TROTEC3550_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        out->state, IR_TROTEC_STATE_BYTES)) return false;

    out->bits         = IR_TROTEC_BITS;
    out->state_nbytes = IR_TROTEC_STATE_BYTES;
    return true;
}
