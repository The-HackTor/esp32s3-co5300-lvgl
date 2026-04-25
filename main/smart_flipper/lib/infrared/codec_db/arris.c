#include "arris.h"
#include "codec_match.h"

#include <string.h>

static uint8_t arris_sum_nibbles(uint64_t data)
{
    uint8_t sum = 0;
    while(data) {
        sum += (uint8_t)(data & 0xF);
        data >>= 4;
    }
    return sum & 0xF;
}

bool ir_arris_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    if(n_timings < 2) return false;

    size_t offset = 0;
    if(!ir_match_mark(timings[offset++], IR_ARRIS_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[offset++], IR_ARRIS_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint64_t value = 0;
    const size_t used = ir_match_manchester(
        timings, n_timings, offset,
        IR_ARRIS_BITS,
        IR_ARRIS_HALF_CLOCK_PERIOD * 2, 0,
        IR_ARRIS_HALF_CLOCK_PERIOD,
        0, 0,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        true,
        &value);
    if(used == 0) return false;

    const uint8_t expected = (uint8_t)(value & 0xF);
    if(expected != arris_sum_nibbles(value >> IR_ARRIS_CHECKSUM_SIZE))
        return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_ARRIS;
    out->bits    = IR_ARRIS_BITS;
    out->value   = value;
    out->address = (uint32_t)((value >> IR_ARRIS_RELEASE_BIT) & 0x1);
    out->command = (uint32_t)((value >> IR_ARRIS_CHECKSUM_SIZE) &
                              ((1u << IR_ARRIS_COMMAND_SIZE) - 1u));
    return true;
}
