#include "bose.h"
#include "codec_match.h"

#include <string.h>

bool ir_bose_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_BOSE_BITS,
        IR_BOSE_HDR_MARK, IR_BOSE_HDR_SPACE,
        IR_BOSE_BIT_MARK, IR_BOSE_ONE_SPACE,
        IR_BOSE_BIT_MARK, IR_BOSE_ZERO_SPACE,
        IR_BOSE_BIT_MARK, IR_BOSE_GAP_MIN,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_BOSE;
    out->bits  = IR_BOSE_BITS;
    out->value = value;
    return true;
}
