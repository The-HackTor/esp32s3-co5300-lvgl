#include "eurom.h"
#include "codec_match.h"

#include <string.h>

bool ir_eurom_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  IR_EUROM_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_EUROM_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_EUROM_STATE_BYTES];
    for(size_t i = 0; i < IR_EUROM_STATE_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_EUROM_BIT_MARK, IR_EUROM_ONE_SPACE,
            IR_EUROM_BIT_MARK, IR_EUROM_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true, true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_EUROM_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_EUROM;
    out->bits         = IR_EUROM_BITS;
    memcpy(out->state, state, IR_EUROM_STATE_BYTES);
    out->state_nbytes = IR_EUROM_STATE_BYTES;
    return true;
}
