#include "bosch.h"
#include "codec_match.h"

#include <string.h>

bool ir_bosch144_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_BOSCH144_STATE_BYTES];
    size_t cursor = 0;

    for(uint8_t section = 0; section < IR_BOSCH144_NR_OF_SECTIONS; section++) {
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_BOSCH_HDR_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(cursor >= n_timings) return false;
        if(!ir_match_space(timings[cursor], IR_BOSCH_HDR_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        for(uint8_t i = 0; i < IR_BOSCH144_BYTES_PER_SECTION; i++) {
            ir_match_result_t r = ir_match_data(
                timings, n_timings, cursor, 8,
                IR_BOSCH_BIT_MARK, IR_BOSCH_ONE_SPACE,
                IR_BOSCH_BIT_MARK, IR_BOSCH_ZERO_SPACE,
                IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
                true,
                true);
            if(!r.success) return false;
            state[section * IR_BOSCH144_BYTES_PER_SECTION + i] = (uint8_t)r.data;
            cursor += r.used;
        }

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_BOSCH_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        const bool last = (section == IR_BOSCH144_NR_OF_SECTIONS - 1);
        const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
        if(last) {
            if(!ir_match_at_least(measured, IR_BOSCH_FOOTER_SPACE,
                                  IR_DEFAULT_TOLERANCE, 0)) return false;
        } else {
            if(cursor >= n_timings) return false;
            if(!ir_match_space(measured, IR_BOSCH_FOOTER_SPACE,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
            cursor++;
        }
    }

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_BOSCH144;
    out->bits = IR_BOSCH144_BITS;
    memcpy(out->state, state, IR_BOSCH144_STATE_BYTES);
    out->state_nbytes = IR_BOSCH144_STATE_BYTES;
    return true;
}
