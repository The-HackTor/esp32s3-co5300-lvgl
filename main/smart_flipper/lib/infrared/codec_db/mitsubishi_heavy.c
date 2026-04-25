#include "mitsubishi_heavy.h"
#include "codec_match.h"

#include <string.h>

static bool decode_bytes(const uint16_t *timings, size_t n_timings,
                         uint8_t *state, uint16_t nbytes)
{
    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_MITSUBISHI_HEAVY_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_MITSUBISHI_HEAVY_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, 0)) return false;
    cursor++;

    for(uint16_t b = 0; b < nbytes; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_MITSUBISHI_HEAVY_BIT_MARK, IR_MITSUBISHI_HEAVY_ONE_SPACE,
            IR_MITSUBISHI_HEAVY_BIT_MARK, IR_MITSUBISHI_HEAVY_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, 0,
            false, true);
        if(!r.success) return false;
        cursor += r.used;
        state[b] = (uint8_t)r.data;
    }
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_MITSUBISHI_HEAVY_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_MITSUBISHI_HEAVY_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }
    return true;
}

bool ir_mitsubishi_heavy_88_decode(const uint16_t *timings, size_t n_timings,
                                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI_HEAVY_88;

    if(!decode_bytes(timings, n_timings, out->state,
                     IR_MITSUBISHI_HEAVY_88_STATE_BYTES)) return false;
    out->bits         = IR_MITSUBISHI_HEAVY_88_BITS;
    out->state_nbytes = IR_MITSUBISHI_HEAVY_88_STATE_BYTES;
    return true;
}

bool ir_mitsubishi_heavy_152_decode(const uint16_t *timings, size_t n_timings,
                                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MITSUBISHI_HEAVY_152;

    if(!decode_bytes(timings, n_timings, out->state,
                     IR_MITSUBISHI_HEAVY_152_STATE_BYTES)) return false;
    out->bits         = IR_MITSUBISHI_HEAVY_152_BITS;
    out->state_nbytes = IR_MITSUBISHI_HEAVY_152_STATE_BYTES;
    return true;
}
