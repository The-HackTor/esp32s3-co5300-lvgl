#include "wowwee.h"
#include "codec_match.h"

#include <string.h>

bool ir_wowwee_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_WOWWEE_BITS,
        IR_WOWWEE_HDR_MARK, IR_WOWWEE_HDR_SPACE,
        IR_WOWWEE_BIT_MARK, IR_WOWWEE_ONE_SPACE,
        IR_WOWWEE_BIT_MARK, IR_WOWWEE_ZERO_SPACE,
        IR_WOWWEE_BIT_MARK, IR_WOWWEE_MSG_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_WOWWEE;
    out->bits  = IR_WOWWEE_BITS;
    out->value = value;
    return true;
}
