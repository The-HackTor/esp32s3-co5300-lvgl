#include "metz.h"
#include "codec_match.h"

#include <string.h>

static uint32_t invert_bits(uint32_t value, uint8_t nbits)
{
    return (~value) & ((1U << nbits) - 1U);
}

bool ir_metz_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_METZ_BITS,
        IR_METZ_HDR_MARK, IR_METZ_HDR_SPACE,
        IR_METZ_BIT_MARK, IR_METZ_ONE_SPACE,
        IR_METZ_BIT_MARK, IR_METZ_ZERO_SPACE,
        IR_METZ_BIT_MARK, IR_METZ_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, 0,
        true,
        &value);
    if(used == 0) return false;

    const uint32_t cmd_low  = (uint32_t)(value & ((1U << IR_METZ_COMMAND_BITS) - 1U));
    const uint32_t cmd_high = (uint32_t)((value >> IR_METZ_COMMAND_BITS) & ((1U << IR_METZ_COMMAND_BITS) - 1U));
    const uint32_t addr_low  = (uint32_t)((value >> (2 * IR_METZ_COMMAND_BITS)) & ((1U << IR_METZ_ADDRESS_BITS) - 1U));
    const uint32_t addr_high = (uint32_t)((value >> (2 * IR_METZ_COMMAND_BITS + IR_METZ_ADDRESS_BITS)) & ((1U << IR_METZ_ADDRESS_BITS) - 1U));

    if(cmd_high != invert_bits(cmd_low, IR_METZ_COMMAND_BITS)) return false;
    if(addr_high != invert_bits(addr_low, IR_METZ_ADDRESS_BITS)) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_METZ;
    out->bits    = IR_METZ_BITS;
    out->value   = value;
    out->address = addr_high;
    out->command = cmd_high;
    return true;
}
