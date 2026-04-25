#include "whynter.h"
#include "codec_match.h"

#include <string.h>

bool ir_whynter_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    if(n_timings < 2) return false;

    if(!ir_match_mark(timings[0], IR_WHYNTER_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[1], IR_WHYNTER_ZERO_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 2,
        IR_WHYNTER_BITS,
        IR_WHYNTER_HDR_MARK, IR_WHYNTER_HDR_SPACE,
        IR_WHYNTER_BIT_MARK, IR_WHYNTER_ONE_SPACE,
        IR_WHYNTER_BIT_MARK, IR_WHYNTER_ZERO_SPACE,
        IR_WHYNTER_BIT_MARK, IR_WHYNTER_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_WHYNTER;
    out->bits  = IR_WHYNTER_BITS;
    out->value = value;
    return true;
}
