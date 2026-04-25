#include "gree.h"
#include "codec_match.h"

#include <string.h>

bool ir_gree_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    uint8_t state[IR_GREE_STATE_BYTES];

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_GREE_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_GREE_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    for(uint8_t i = 0; i < 4; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_GREE_BIT_MARK, IR_GREE_ONE_SPACE,
            IR_GREE_BIT_MARK, IR_GREE_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false,
            true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    ir_match_result_t fr = ir_match_data(
        timings, n_timings, cursor, IR_GREE_BLOCK_FOOTER_BITS,
        IR_GREE_BIT_MARK, IR_GREE_ONE_SPACE,
        IR_GREE_BIT_MARK, IR_GREE_ZERO_SPACE,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        true);
    if(!fr.success) return false;
    if((fr.data & 0x7u) != IR_GREE_BLOCK_FOOTER) return false;
    cursor += fr.used;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_GREE_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_GREE_MSG_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    for(uint8_t i = 4; i < 8; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_GREE_BIT_MARK, IR_GREE_ONE_SPACE,
            IR_GREE_BIT_MARK, IR_GREE_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false,
            true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_GREE_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
    if(!ir_match_at_least(measured, IR_GREE_MSG_SPACE,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_GREE;
    out->bits = IR_GREE_BITS;
    memcpy(out->state, state, IR_GREE_STATE_BYTES);
    out->state_nbytes = IR_GREE_STATE_BYTES;
    return true;
}
