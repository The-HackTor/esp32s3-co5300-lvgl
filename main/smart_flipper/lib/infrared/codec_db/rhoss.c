#include "rhoss.h"
#include "codec_match.h"

#include <string.h>

bool ir_rhoss_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    size_t off = 0;
    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_RHOSS_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_space(timings[off++], IR_RHOSS_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    for(size_t i = 0; i < IR_RHOSS_STATE_LENGTH; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, off, 8,
            IR_RHOSS_BIT_MARK, IR_RHOSS_ONE_SPACE,
            IR_RHOSS_BIT_MARK, IR_RHOSS_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        out->state[i] = (uint8_t)(r.data & 0xFFu);
        off += r.used;
    }

    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_RHOSS_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_space(timings[off++], IR_RHOSS_ZERO_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_RHOSS_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    const uint16_t measured = (off < n_timings) ? timings[off] : 0;
    if(!ir_match_at_least(measured, IR_RHOSS_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    out->id           = IR_CODEC_RHOSS;
    out->bits         = IR_RHOSS_BITS;
    out->state_nbytes = IR_RHOSS_STATE_LENGTH;
    return true;
}
