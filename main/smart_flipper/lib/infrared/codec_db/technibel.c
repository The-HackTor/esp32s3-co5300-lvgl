#include "technibel.h"
#include "codec_match.h"

#include <string.h>

bool ir_technibel_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_TECHNIBEL_AC_BITS,
        IR_TECHNIBEL_AC_HDR_MARK, IR_TECHNIBEL_AC_HDR_SPACE,
        IR_TECHNIBEL_AC_BIT_MARK, IR_TECHNIBEL_AC_ONE_SPACE,
        IR_TECHNIBEL_AC_BIT_MARK, IR_TECHNIBEL_AC_ZERO_SPACE,
        IR_TECHNIBEL_AC_BIT_MARK, IR_TECHNIBEL_AC_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_TECHNIBEL_AC;
    out->bits  = IR_TECHNIBEL_AC_BITS;
    out->value = value;
    return true;
}
