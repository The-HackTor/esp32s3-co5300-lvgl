#include "denon.h"
#include "codec_match.h"

#include <string.h>

bool ir_denon_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_DENON_BITS,
        IR_DENON_HDR_MARK, IR_DENON_HDR_SPACE,
        IR_DENON_BIT_MARK, IR_DENON_ONE_SPACE,
        IR_DENON_BIT_MARK, IR_DENON_ZERO_SPACE,
        IR_DENON_BIT_MARK, 0,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_DENON;
    out->bits  = IR_DENON_BITS;
    out->value = value;
    return true;
}
