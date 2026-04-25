#include "rcmm.h"
#include "codec_match.h"

#include <string.h>

bool ir_rcmm_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_RCMM;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_RCMM_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_RCMM_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    uint64_t value = 0;
    for(uint16_t bits = 0; bits < IR_RCMM_BITS; bits += 2) {
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_RCMM_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        if(cursor >= n_timings) return false;
        const uint16_t s = timings[cursor];
        value <<= 2;
        if(ir_match(s, IR_RCMM_ZERO_SPACE, IR_DEFAULT_TOLERANCE, 0)) {
            value |= 0;
        } else if(ir_match(s, IR_RCMM_ONE_SPACE, IR_DEFAULT_TOLERANCE, 0)) {
            value |= 1;
        } else if(ir_match(s, IR_RCMM_TWO_SPACE, IR_RCMM_TOLERANCE, 0)) {
            value |= 2;
        } else if(ir_match(s, IR_RCMM_THREE_SPACE, IR_RCMM_TOLERANCE, 0)) {
            value |= 3;
        } else {
            return false;
        }
        cursor++;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_RCMM_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_RCMM_MIN_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }

    out->bits  = IR_RCMM_BITS;
    out->value = value;
    return true;
}
