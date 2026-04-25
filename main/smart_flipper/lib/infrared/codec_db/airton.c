#include "airton.h"
#include "codec_match.h"

#include <string.h>

bool ir_airton_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_AIRTON_BITS,
        IR_AIRTON_HDR_MARK, IR_AIRTON_HDR_SPACE,
        IR_AIRTON_BIT_MARK, IR_AIRTON_ONE_SPACE,
        IR_AIRTON_BIT_MARK, IR_AIRTON_ZERO_SPACE,
        IR_AIRTON_BIT_MARK, IR_AIRTON_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_AIRTON;
    out->bits  = IR_AIRTON_BITS;
    out->value = value;
    return true;
}
