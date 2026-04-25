#include "sherwood.h"
#include "codec_match.h"

#include <string.h>

bool ir_sherwood_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_SHERWOOD_BITS,
        IR_SHERWOOD_HDR_MARK, IR_SHERWOOD_HDR_SPACE,
        IR_SHERWOOD_BIT_MARK, IR_SHERWOOD_ONE_SPACE,
        IR_SHERWOOD_BIT_MARK, IR_SHERWOOD_ZERO_SPACE,
        IR_SHERWOOD_BIT_MARK, IR_SHERWOOD_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_SHERWOOD;
    out->bits    = IR_SHERWOOD_BITS;
    out->value   = value;
    out->address = (uint32_t)(value >> 16);
    out->command = (uint32_t)(value & 0xFFFF);
    return true;
}
