#include "kelvinator.h"
#include "codec_match.h"

#include <string.h>

static bool read_bytes_lsb(const uint16_t *timings, size_t n_timings,
                           size_t *cursor, uint16_t nbytes, uint8_t *out)
{
    for(uint16_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, *cursor, 8,
            IR_KELVINATOR_BIT_MARK, IR_KELVINATOR_ONE_SPACE,
            IR_KELVINATOR_BIT_MARK, IR_KELVINATOR_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false,
            true);
        if(!r.success) return false;
        out[i] = (uint8_t)r.data;
        *cursor += r.used;
    }
    return true;
}

bool ir_kelvinator_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_KELVINATOR_STATE_BYTES];
    size_t cursor = 0;

    for(uint8_t section = 0; section < 2; section++) {
        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_KELVINATOR_HDR_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(cursor >= n_timings) return false;
        if(!ir_match_space(timings[cursor], IR_KELVINATOR_HDR_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        if(!read_bytes_lsb(timings, n_timings, &cursor, 4, state + section * 8))
            return false;

        ir_match_result_t fr = ir_match_data(
            timings, n_timings, cursor, IR_KELVINATOR_CMD_FOOTER_BITS,
            IR_KELVINATOR_BIT_MARK, IR_KELVINATOR_ONE_SPACE,
            IR_KELVINATOR_BIT_MARK, IR_KELVINATOR_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false,
            true);
        if(!fr.success) return false;
        if((fr.data & 0x7u) != IR_KELVINATOR_CMD_FOOTER) return false;
        cursor += fr.used;

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_KELVINATOR_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;
        if(cursor >= n_timings) return false;
        if(!ir_match_space(timings[cursor], IR_KELVINATOR_GAP_SPACE,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        if(!read_bytes_lsb(timings, n_timings, &cursor, 4,
                           state + section * 8 + 4)) return false;

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_KELVINATOR_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        const uint32_t expected_gap = IR_KELVINATOR_GAP_SPACE * 2u;
        const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
        if(section == 0) {
            if(cursor >= n_timings) return false;
            if(!ir_match_space(measured, expected_gap,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
            cursor++;
        } else {
            if(!ir_match_at_least(measured, expected_gap,
                                  IR_DEFAULT_TOLERANCE, 0)) return false;
        }
    }

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_KELVINATOR;
    out->bits = IR_KELVINATOR_BITS;
    memcpy(out->state, state, IR_KELVINATOR_STATE_BYTES);
    out->state_nbytes = IR_KELVINATOR_STATE_BYTES;
    return true;
}
