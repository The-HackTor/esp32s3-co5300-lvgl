#include "whirlpool.h"
#include "codec_match.h"

#include <string.h>

bool ir_whirlpool_ac_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_WHIRLPOOL_AC;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_WHIRLPOOL_AC_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_WHIRLPOOL_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    static const uint8_t section_sizes[3] = {6, 8, 7};
    uint16_t pos = 0;
    for(uint8_t s = 0; s < 3; s++) {
        for(uint8_t b = 0; b < section_sizes[s]; b++) {
            ir_match_result_t r = ir_match_data(
                timings, n_timings, cursor, 8,
                IR_WHIRLPOOL_AC_BIT_MARK, IR_WHIRLPOOL_AC_ONE_SPACE,
                IR_WHIRLPOOL_AC_BIT_MARK, IR_WHIRLPOOL_AC_ZERO_SPACE,
                IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
                false, true);
            if(!r.success) return false;
            cursor += r.used;
            out->state[pos + b] = (uint8_t)r.data;
        }
        pos += section_sizes[s];
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_WHIRLPOOL_AC_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(s < 2) {
            if(cursor >= n_timings) return false;
            if(!ir_match_space(timings[cursor], IR_WHIRLPOOL_AC_GAP,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
            cursor++;
        } else {
            if(cursor < n_timings) {
                if(!ir_match_at_least(timings[cursor], IR_WHIRLPOOL_AC_GAP,
                                      IR_DEFAULT_TOLERANCE, 0)) return false;
            }
        }
    }

    if(pos != IR_WHIRLPOOL_AC_STATE_BYTES) return false;
    out->bits         = IR_WHIRLPOOL_AC_BITS;
    out->state_nbytes = IR_WHIRLPOOL_AC_STATE_BYTES;
    return true;
}
