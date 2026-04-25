#include "vestel.h"
#include "codec_match.h"

#include <string.h>

bool ir_vestel_ac_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_VESTEL_AC;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_VESTEL_AC_BITS,
        IR_VESTEL_AC_HDR_MARK, IR_VESTEL_AC_HDR_SPACE,
        IR_VESTEL_AC_BIT_MARK, IR_VESTEL_AC_ONE_SPACE,
        IR_VESTEL_AC_BIT_MARK, IR_VESTEL_AC_ZERO_SPACE,
        IR_VESTEL_AC_BIT_MARK, 0,
        false,
        IR_VESTEL_AC_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    out->bits  = IR_VESTEL_AC_BITS;
    out->value = value;
    return true;
}
