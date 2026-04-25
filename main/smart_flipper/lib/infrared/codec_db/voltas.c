#include "voltas.h"
#include "codec_match.h"

#include <string.h>

bool ir_voltas_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_VOLTAS;

    size_t cursor = 0;
    for(uint16_t b = 0; b < IR_VOLTAS_STATE_BYTES; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_VOLTAS_BIT_MARK, IR_VOLTAS_ONE_SPACE,
            IR_VOLTAS_BIT_MARK, IR_VOLTAS_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        cursor += r.used;
        out->state[b] = (uint8_t)r.data;
    }
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_VOLTAS_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_VOLTAS_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }

    out->bits         = IR_VOLTAS_BITS;
    out->state_nbytes = IR_VOLTAS_STATE_BYTES;
    return true;
}
