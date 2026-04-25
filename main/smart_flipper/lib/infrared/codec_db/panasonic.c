#include "panasonic.h"
#include "codec_match.h"

#include <string.h>

bool ir_panasonic_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_PANASONIC_BITS,
        IR_PANASONIC_HDR_MARK, IR_PANASONIC_HDR_SPACE,
        IR_PANASONIC_BIT_MARK, IR_PANASONIC_ONE_SPACE,
        IR_PANASONIC_BIT_MARK, IR_PANASONIC_ZERO_SPACE,
        IR_PANASONIC_BIT_MARK, IR_PANASONIC_END_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_PANASONIC;
    out->bits    = IR_PANASONIC_BITS;
    out->value   = value;
    out->address = (uint32_t)(value >> 32);
    out->command = (uint32_t)value;
    return true;
}

static bool decode_ac_section(const uint16_t *timings, size_t n_timings,
                              size_t *cursor, uint16_t section_bits,
                              uint32_t footer_space, bool at_least_footer,
                              uint8_t *state_out)
{
    if(*cursor >= n_timings) return false;
    if(!ir_match_mark(timings[*cursor], IR_PANASONIC_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    (*cursor)++;
    if(*cursor >= n_timings) return false;
    if(!ir_match_space(timings[*cursor], IR_PANASONIC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    (*cursor)++;

    const uint16_t nbytes = section_bits / 8;
    for(uint16_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, *cursor, 8,
            IR_PANASONIC_BIT_MARK, IR_PANASONIC_ONE_SPACE,
            IR_PANASONIC_BIT_MARK, IR_PANASONIC_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false,
            true);
        if(!r.success) return false;
        state_out[i] = (uint8_t)r.data;
        *cursor += r.used;
    }

    if(*cursor >= n_timings) return false;
    if(!ir_match_mark(timings[*cursor], IR_PANASONIC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    (*cursor)++;

    const uint16_t measured = (*cursor < n_timings) ? timings[*cursor] : 0;
    if(at_least_footer) {
        if(!ir_match_at_least(measured, footer_space,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    } else {
        if(!ir_match_space(measured, footer_space,
                           IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    }
    if(*cursor < n_timings) (*cursor)++;
    return true;
}

bool ir_panasonic_ac_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_PANASONIC_AC_STATE_BYTES];
    size_t cursor = 0;

    if(!decode_ac_section(timings, n_timings, &cursor,
                          IR_PANASONIC_AC_SECTION1_LEN * 8,
                          IR_PANASONIC_AC_SECTION_GAP, false, state))
        return false;

    const uint16_t remaining_bits =
        IR_PANASONIC_AC_BITS - IR_PANASONIC_AC_SECTION1_LEN * 8;
    if(!decode_ac_section(timings, n_timings, &cursor, remaining_bits,
                          IR_PANASONIC_MIN_GAP, true,
                          state + IR_PANASONIC_AC_SECTION1_LEN))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_PANASONIC_AC;
    out->bits = IR_PANASONIC_AC_BITS;
    memcpy(out->state, state, IR_PANASONIC_AC_STATE_BYTES);
    out->state_nbytes = IR_PANASONIC_AC_STATE_BYTES;
    return true;
}

bool ir_panasonic_ac32_decode(const uint16_t *timings, size_t n_timings,
                              ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    (void)n_timings;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_UNKNOWN;
    return false;
}
