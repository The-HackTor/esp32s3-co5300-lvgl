#include "toshiba.h"
#include "codec_match.h"

#include <string.h>

static bool try_decode(const uint16_t *timings, size_t n_timings,
                       uint16_t nbytes, ir_decode_result_t *out)
{
    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TOSHIBA_AC_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_TOSHIBA_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    for(uint16_t b = 0; b < nbytes; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_TOSHIBA_AC_BIT_MARK, IR_TOSHIBA_AC_ONE_SPACE,
            IR_TOSHIBA_AC_BIT_MARK, IR_TOSHIBA_AC_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        cursor += r.used;
        out->state[b] = (uint8_t)r.data;
    }
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TOSHIBA_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_TOSHIBA_AC_MIN_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }
    out->bits         = (uint16_t)(nbytes * 8);
    out->state_nbytes = nbytes;
    return true;
}

bool ir_toshiba_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_TOSHIBA_AC;

    static const uint16_t lengths[3] = {
        IR_TOSHIBA_AC_STATE_BYTES,
        IR_TOSHIBA_AC_STATE_BYTES_LONG,
        IR_TOSHIBA_AC_STATE_BYTES_SHORT,
    };
    for(size_t i = 0; i < 3; i++) {
        ir_decode_result_t tmp;
        memset(&tmp, 0, sizeof(tmp));
        tmp.id = IR_CODEC_UNKNOWN;
        if(try_decode(timings, n_timings, lengths[i], &tmp)) {
            *out = tmp;
            return true;
        }
    }
    return false;
}
