#include "elitescreens.h"
#include "codec_match.h"

#include <string.h>

bool ir_elitescreens_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    uint64_t value = 0;
    const size_t used = ir_match_const_bit_time(
        timings, n_timings, 0,
        IR_ELITESCREENS_BITS,
        0, 0,
        IR_ELITESCREENS_ONE_PULSE, IR_ELITESCREENS_ZERO_PULSE,
        0, IR_ELITESCREENS_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    out->id    = IR_CODEC_ELITESCREENS;
    out->bits  = IR_ELITESCREENS_BITS;
    out->value = value;
    return true;
}
