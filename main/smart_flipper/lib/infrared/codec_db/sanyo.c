#include "sanyo.h"
#include "codec_match.h"

#include <string.h>

static bool decode_ac_state(const uint16_t *timings, size_t n_timings,
                            uint16_t hdr_mark, uint16_t hdr_space,
                            uint16_t bit_mark,
                            uint16_t one_space, uint16_t zero_space,
                            uint8_t tolerance, size_t nbytes,
                            uint8_t *state)
{
    size_t cursor = 0;
    if(cursor + 1 >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++],  hdr_mark, tolerance, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], hdr_space, tolerance, IR_MARK_EXCESS)) return false;

    for(size_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            bit_mark, one_space,
            bit_mark, zero_space,
            tolerance, IR_MARK_EXCESS,
            false, true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], bit_mark, tolerance, IR_MARK_EXCESS)) return false;
    return true;
}

bool ir_sanyo_lc7461_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_SANYO_LC7461_BITS,
        IR_SANYO_LC7461_HDR_MARK, IR_SANYO_LC7461_HDR_SPACE,
        IR_SANYO_LC7461_BIT_MARK, IR_SANYO_LC7461_ONE_SPACE,
        IR_SANYO_LC7461_BIT_MARK, IR_SANYO_LC7461_ZERO_SPACE,
        IR_SANYO_LC7461_BIT_MARK, IR_SANYO_LC7461_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    const uint32_t addr_mask = (1U << IR_SANYO_LC7461_ADDRESS_BITS) - 1U;
    const uint32_t cmd_mask  = (1U << IR_SANYO_LC7461_COMMAND_BITS) - 1U;
    const uint32_t address = (uint32_t)(value >> (IR_SANYO_LC7461_BITS - IR_SANYO_LC7461_ADDRESS_BITS));
    const uint32_t command = (uint32_t)((value >> IR_SANYO_LC7461_COMMAND_BITS) & cmd_mask);
    const uint32_t inverted_address =
        (uint32_t)((value >> (IR_SANYO_LC7461_COMMAND_BITS * 2)) & addr_mask);
    const uint32_t inverted_command = (uint32_t)(value & cmd_mask);

    if((address ^ addr_mask) != inverted_address) return false;
    if((command ^ cmd_mask) != inverted_command) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_SANYO_LC7461;
    out->bits    = IR_SANYO_LC7461_BITS;
    out->value   = value;
    out->address = address;
    out->command = command;
    return true;
}

bool ir_sanyo_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_SANYO_AC_STATE_BYTES];
    if(!decode_ac_state(timings, n_timings,
                        IR_SANYO_AC_HDR_MARK, IR_SANYO_AC_HDR_SPACE,
                        IR_SANYO_AC_BIT_MARK,
                        IR_SANYO_AC_ONE_SPACE, IR_SANYO_AC_ZERO_SPACE,
                        IR_DEFAULT_TOLERANCE,
                        IR_SANYO_AC_STATE_BYTES, state)) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SANYO_AC;
    out->bits         = IR_SANYO_AC_BITS;
    memcpy(out->state, state, IR_SANYO_AC_STATE_BYTES);
    out->state_nbytes = IR_SANYO_AC_STATE_BYTES;
    return true;
}

bool ir_sanyo_ac88_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_SANYO_AC88_STATE_BYTES];
    if(!decode_ac_state(timings, n_timings,
                        IR_SANYO_AC88_HDR_MARK, IR_SANYO_AC88_HDR_SPACE,
                        IR_SANYO_AC88_BIT_MARK,
                        IR_SANYO_AC88_ONE_SPACE, IR_SANYO_AC88_ZERO_SPACE,
                        IR_SANYO_AC88_TOLERANCE,
                        IR_SANYO_AC88_STATE_BYTES, state)) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SANYO_AC88;
    out->bits         = IR_SANYO_AC88_BITS;
    memcpy(out->state, state, IR_SANYO_AC88_STATE_BYTES);
    out->state_nbytes = IR_SANYO_AC88_STATE_BYTES;
    return true;
}

bool ir_sanyo_ac152_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    if(IR_SANYO_AC152_STATE_BYTES > IR_DECODE_STATE_MAX) return false;

    uint8_t state[IR_SANYO_AC152_STATE_BYTES];
    if(!decode_ac_state(timings, n_timings,
                        IR_SANYO_AC152_HDR_MARK, IR_SANYO_AC152_HDR_SPACE,
                        IR_SANYO_AC152_BIT_MARK,
                        IR_SANYO_AC152_ONE_SPACE, IR_SANYO_AC152_ZERO_SPACE,
                        IR_SANYO_AC152_TOLERANCE,
                        IR_SANYO_AC152_STATE_BYTES, state)) return false;

    memset(out, 0, sizeof(*out));
    out->id           = IR_CODEC_SANYO_AC152;
    out->bits         = IR_SANYO_AC152_BITS;
    memcpy(out->state, state, IR_SANYO_AC152_STATE_BYTES);
    out->state_nbytes = IR_SANYO_AC152_STATE_BYTES;
    return true;
}
