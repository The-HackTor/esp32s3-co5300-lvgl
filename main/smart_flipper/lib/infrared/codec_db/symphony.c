#include "symphony.h"
#include "codec_match.h"

#include <string.h>

bool ir_symphony_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_SYMPHONY_BITS,
        0, 0,
        IR_SYMPHONY_ONE_MARK,  IR_SYMPHONY_ONE_SPACE,
        IR_SYMPHONY_ZERO_MARK, IR_SYMPHONY_ZERO_SPACE,
        0, IR_SYMPHONY_FOOTER_GAP,
        true,
        IR_DEFAULT_TOLERANCE, 0,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_SYMPHONY;
    out->bits  = IR_SYMPHONY_BITS;
    out->value = value;
    return true;
}
