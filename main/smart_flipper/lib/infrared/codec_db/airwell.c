#include "airwell.h"
#include "codec_match.h"

#include <string.h>

bool ir_airwell_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_manchester(
        timings, n_timings, 0,
        IR_AIRWELL_BITS,
        IR_AIRWELL_HDR_MARK, IR_AIRWELL_HDR_MARK,
        IR_AIRWELL_HALF_CLOCK_PERIOD,
        IR_AIRWELL_HDR_MARK, IR_AIRWELL_HDR_SPACE,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_AIRWELL;
    out->bits  = IR_AIRWELL_BITS;
    out->value = value;
    return true;
}
