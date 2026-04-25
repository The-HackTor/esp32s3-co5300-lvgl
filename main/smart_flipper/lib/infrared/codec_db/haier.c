#include "haier.h"
#include "codec_match.h"

#include <string.h>

static bool haier_decode(const uint16_t *timings, size_t n_timings,
                         uint16_t nbytes, ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    if(nbytes > IR_DECODE_STATE_MAX) return false;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_HAIER_AC_HDR,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_HAIER_AC_HDR,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_HAIER_AC_HDR,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_HAIER_AC_HDR_GAP,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    uint8_t state[IR_DECODE_STATE_MAX];
    for(uint16_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_HAIER_AC_BIT_MARK, IR_HAIER_AC_ONE_SPACE,
            IR_HAIER_AC_BIT_MARK, IR_HAIER_AC_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true,
            true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_HAIER_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
    if(!ir_match_at_least(measured, IR_HAIER_AC_MIN_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_UNKNOWN;
    out->bits = (uint16_t)(nbytes * 8);
    memcpy(out->state, state, nbytes);
    out->state_nbytes = nbytes;
    return true;
}

bool ir_haier_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    return haier_decode(timings, n_timings, IR_HAIER_AC_STATE_BYTES, out);
}

bool ir_haier_ac_yrw02_decode(const uint16_t *timings, size_t n_timings,
                              ir_decode_result_t *out)
{
    return haier_decode(timings, n_timings, IR_HAIER_AC_YRW02_STATE_BYTES, out);
}

bool ir_haier_ac160_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    return haier_decode(timings, n_timings, IR_HAIER_AC160_STATE_BYTES, out);
}

bool ir_haier_ac176_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    return haier_decode(timings, n_timings, IR_HAIER_AC176_STATE_BYTES, out);
}
