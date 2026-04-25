#include "tcl.h"
#include "codec_match.h"

#include <string.h>

#define TCL112_TOL (IR_DEFAULT_TOLERANCE + IR_TCL112_TOLERANCE_EXTRA)

bool ir_tcl112_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TCL112_HDR_MARK,
                      TCL112_TOL, 0)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_TCL112_HDR_SPACE,
                       TCL112_TOL, 0)) return false;
    cursor++;

    uint8_t state[IR_TCL112_STATE_BYTES];
    for(uint16_t i = 0; i < IR_TCL112_STATE_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_TCL112_BIT_MARK, IR_TCL112_ONE_SPACE,
            IR_TCL112_BIT_MARK, IR_TCL112_ZERO_SPACE,
            TCL112_TOL, 0,
            false,
            true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TCL112_BIT_MARK,
                      TCL112_TOL, 0)) return false;
    cursor++;
    const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
    if(!ir_match_at_least(measured, IR_TCL112_GAP, TCL112_TOL, 0)) return false;

    if(state[0] != 0x23 || state[1] != 0xCB || state[2] != 0x26) return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_TCL112;
    out->bits = IR_TCL112_BITS;
    memcpy(out->state, state, IR_TCL112_STATE_BYTES);
    out->state_nbytes = IR_TCL112_STATE_BYTES;
    return true;
}

bool ir_tcl96_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_TCL96_AC;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TCL96_AC_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_TCL96_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    uint8_t state[IR_TCL96_AC_STATE_BYTES];
    uint8_t  current = 0;
    for(uint16_t bits_so_far = 0; bits_so_far < IR_TCL96_AC_BITS; bits_so_far += 2) {
        if((bits_so_far % 8) == 0) current = 0;
        else current = (uint8_t)(current << 2);

        if(cursor >= n_timings) return false;
        if(!ir_match_mark(timings[cursor], IR_TCL96_AC_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        cursor++;

        if(cursor >= n_timings) return false;
        const uint16_t s = timings[cursor];
        uint8_t code;
        if(ir_match_space(s, IR_TCL96_AC_ZERO_ZERO_SPACE,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) code = 0;
        else if(ir_match_space(s, IR_TCL96_AC_ZERO_ONE_SPACE,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) code = 1;
        else if(ir_match_space(s, IR_TCL96_AC_ONE_ZERO_SPACE,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) code = 2;
        else if(ir_match_space(s, IR_TCL96_AC_ONE_ONE_SPACE,
                               IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) code = 3;
        else return false;
        cursor++;

        current = (uint8_t)(current | code);
        state[bits_so_far / 8] = current;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_TCL96_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_TCL96_AC_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }

    out->bits = IR_TCL96_AC_BITS;
    memcpy(out->state, state, IR_TCL96_AC_STATE_BYTES);
    out->state_nbytes = IR_TCL96_AC_STATE_BYTES;
    return true;
}
