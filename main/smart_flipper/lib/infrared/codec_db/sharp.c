#include "sharp.h"
#include "codec_match.h"

#include <string.h>

bool ir_sharp_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_SHARP_BITS,
        0, 0,
        IR_SHARP_BIT_MARK, IR_SHARP_ONE_SPACE,
        IR_SHARP_BIT_MARK, IR_SHARP_ZERO_SPACE,
        IR_SHARP_BIT_MARK, IR_SHARP_GAP,
        true,
        35, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    if(((value & 0b10) >> 1) != 1) return false;
    if(value & 0b1) return false;

    const uint64_t address_mask = (1ULL << IR_SHARP_ADDRESS_BITS) - 1ULL;
    const uint64_t command_mask = (1ULL << IR_SHARP_COMMAND_BITS) - 1ULL;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_SHARP;
    out->bits    = IR_SHARP_BITS;
    out->value   = value;
    out->address = (uint32_t)(ir_reverse_bits(value, IR_SHARP_BITS) & address_mask);
    out->command = (uint32_t)ir_reverse_bits((value >> 2) & command_mask,
                                              IR_SHARP_COMMAND_BITS);
    return true;
}

bool ir_sharp_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  IR_SHARP_AC_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_SHARP_AC_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_SHARP_AC_STATE_BYTES];
    for(size_t i = 0; i < IR_SHARP_AC_STATE_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_SHARP_AC_BIT_MARK, IR_SHARP_AC_ONE_SPACE,
            IR_SHARP_AC_BIT_MARK, IR_SHARP_AC_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_SHARP_AC_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SHARP_AC;
    out->bits         = IR_SHARP_AC_BITS;
    memcpy(out->state, state, IR_SHARP_AC_STATE_BYTES);
    out->state_nbytes = IR_SHARP_AC_STATE_BYTES;
    return true;
}
