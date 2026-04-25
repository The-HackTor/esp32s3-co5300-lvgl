#include "fujitsu.h"
#include "codec_match.h"

#include <string.h>

#define FUJITSU_TOL (IR_DEFAULT_TOLERANCE + IR_FUJITSU_AC_EXTRA_TOLERANCE)

bool ir_fujitsu_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_FUJITSU_AC_HDR_MARK,
                      FUJITSU_TOL, 0)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_FUJITSU_AC_HDR_SPACE,
                       FUJITSU_TOL, 0)) return false;
    cursor++;

    uint8_t state[IR_FUJITSU_AC_STATE_MAX];
    uint16_t bytes_read = 0;

    for(uint16_t i = 0; i < IR_FUJITSU_AC_PREFIX_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_FUJITSU_AC_BIT_MARK, IR_FUJITSU_AC_ONE_SPACE,
            IR_FUJITSU_AC_BIT_MARK, IR_FUJITSU_AC_ZERO_SPACE,
            FUJITSU_TOL, 0,
            false,
            true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
        bytes_read++;
    }

    if(state[0] != 0x14 || state[1] != 0x63) return false;

    while(bytes_read < IR_FUJITSU_AC_STATE_MAX && cursor + 16 <= n_timings) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_FUJITSU_AC_BIT_MARK, IR_FUJITSU_AC_ONE_SPACE,
            IR_FUJITSU_AC_BIT_MARK, IR_FUJITSU_AC_ZERO_SPACE,
            FUJITSU_TOL, 0,
            false,
            true);
        if(!r.success) break;
        state[bytes_read++] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_FUJITSU_AC_BIT_MARK,
                      FUJITSU_TOL, 0)) return false;
    cursor++;

    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_FUJITSU_AC_MIN_GAP,
                              FUJITSU_TOL, 0)) return false;
    }

    if(bytes_read < IR_FUJITSU_AC_STATE_MIN - 1) return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_FUJITSU_AC;
    out->bits = (uint16_t)(bytes_read * 8);
    memcpy(out->state, state, bytes_read);
    out->state_nbytes = bytes_read;
    return true;
}
