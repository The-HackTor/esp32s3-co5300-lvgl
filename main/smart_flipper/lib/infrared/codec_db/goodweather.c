#include "goodweather.h"
#include "codec_match.h"

#include <string.h>

bool ir_goodweather_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    const uint8_t tol = IR_DEFAULT_TOLERANCE + IR_GOODWEATHER_EXTRA_TOLERANCE;
    const uint16_t total_bytes = IR_GOODWEATHER_BITS / 8;
    if(total_bytes > IR_DECODE_STATE_MAX) return false;

    if(n_timings < 2) return false;
    size_t cursor = 0;
    if(!ir_match_mark(timings[cursor], IR_GOODWEATHER_HDR_MARK, tol, IR_MARK_EXCESS)) return false;
    cursor++;
    if(!ir_match_space(timings[cursor], IR_GOODWEATHER_HDR_SPACE, tol, IR_MARK_EXCESS)) return false;
    cursor++;

    for(uint16_t b = 0; b < total_bytes; b++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_GOODWEATHER_BIT_MARK, IR_GOODWEATHER_ONE_SPACE,
            IR_GOODWEATHER_BIT_MARK, IR_GOODWEATHER_ZERO_SPACE,
            tol, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        cursor += r.used;
        const uint8_t byte_val = (uint8_t)r.data;

        ir_match_result_t inv = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_GOODWEATHER_BIT_MARK, IR_GOODWEATHER_ONE_SPACE,
            IR_GOODWEATHER_BIT_MARK, IR_GOODWEATHER_ZERO_SPACE,
            tol, IR_MARK_EXCESS,
            false, true);
        if(!inv.success) return false;
        cursor += inv.used;
        const uint8_t inv_val = (uint8_t)inv.data;

        if((uint8_t)(byte_val ^ inv_val) != 0xFF) return false;
        out->state[b] = byte_val;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_GOODWEATHER_BIT_MARK, tol, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_GOODWEATHER_HDR_SPACE, tol, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_GOODWEATHER_BIT_MARK, tol, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_GOODWEATHER_HDR_SPACE, tol, 0)) return false;
    }

    uint64_t value = 0;
    for(uint16_t b = 0; b < total_bytes; b++) {
        value |= ((uint64_t)out->state[b]) << (b * 8);
    }

    out->id           = IR_CODEC_GOODWEATHER;
    out->bits         = IR_GOODWEATHER_BITS;
    out->value        = value;
    out->state_nbytes = total_bytes;
    return true;
}
