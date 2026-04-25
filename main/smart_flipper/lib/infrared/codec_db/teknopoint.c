#include "teknopoint.h"
#include "codec_match.h"

#include <string.h>

bool ir_teknopoint_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  IR_TEKNOPOINT_HDR_MARK,
                      IR_TEKNOPOINT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_TEKNOPOINT_HDR_SPACE,
                       IR_TEKNOPOINT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_TEKNOPOINT_STATE_BYTES];
    for(size_t i = 0; i < IR_TEKNOPOINT_STATE_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_TEKNOPOINT_BIT_MARK, IR_TEKNOPOINT_ONE_SPACE,
            IR_TEKNOPOINT_BIT_MARK, IR_TEKNOPOINT_ZERO_SPACE,
            IR_TEKNOPOINT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_TEKNOPOINT_BIT_MARK,
                      IR_TEKNOPOINT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t sum = 0;
    for(size_t i = 0; i < IR_TEKNOPOINT_STATE_BYTES - 1; i++) sum += state[i];
    if(sum != state[IR_TEKNOPOINT_STATE_BYTES - 1]) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_TEKNOPOINT;
    out->bits         = IR_TEKNOPOINT_BITS;
    memcpy(out->state, state, IR_TEKNOPOINT_STATE_BYTES);
    out->state_nbytes = IR_TEKNOPOINT_STATE_BYTES;
    return true;
}
