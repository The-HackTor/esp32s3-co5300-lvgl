#include "gorenje.h"
#include "codec_match.h"

#include <string.h>

bool ir_gorenje_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_GORENJE_BITS,
        0, 0,
        IR_GORENJE_BIT_MARK, IR_GORENJE_ONE_SPACE,
        IR_GORENJE_BIT_MARK, IR_GORENJE_ZERO_SPACE,
        IR_GORENJE_BIT_MARK, IR_GORENJE_MIN_GAP,
        true,
        IR_GORENJE_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_GORENJE;
    out->bits  = IR_GORENJE_BITS;
    out->value = value;
    return true;
}
