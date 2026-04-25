#include "york.h"
#include "codec_match.h"

#include <string.h>

bool ir_york_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_YORK;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_YORK_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_YORK_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    for(uint16_t b = 0; b < IR_YORK_STATE_BYTES; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_YORK_BIT_MARK, IR_YORK_ONE_SPACE,
            IR_YORK_BIT_MARK, IR_YORK_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        cursor += r.used;
        out->state[b] = (uint8_t)r.data;
    }
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_YORK_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    out->bits         = IR_YORK_BITS;
    out->state_nbytes = IR_YORK_STATE_BYTES;
    return true;
}
