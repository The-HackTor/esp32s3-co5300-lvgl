#include "nikai.h"
#include "codec_match.h"

#include <string.h>

bool ir_nikai_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_NIKAI_BITS,
        IR_NIKAI_HDR_MARK, IR_NIKAI_HDR_SPACE,
        IR_NIKAI_BIT_MARK, IR_NIKAI_ONE_SPACE,
        IR_NIKAI_BIT_MARK, IR_NIKAI_ZERO_SPACE,
        IR_NIKAI_BIT_MARK, IR_NIKAI_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_NIKAI;
    out->bits    = IR_NIKAI_BITS;
    out->value   = value;
    out->address = 0;
    out->command = 0;
    return true;
}
