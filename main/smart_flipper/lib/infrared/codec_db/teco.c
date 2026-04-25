#include "teco.h"
#include "codec_match.h"

#include <string.h>

bool ir_teco_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_TECO_BITS,
        IR_TECO_HDR_MARK, IR_TECO_HDR_SPACE,
        IR_TECO_BIT_MARK, IR_TECO_ONE_SPACE,
        IR_TECO_BIT_MARK, IR_TECO_ZERO_SPACE,
        IR_TECO_BIT_MARK, IR_TECO_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        false,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_TECO;
    out->bits  = IR_TECO_BITS;
    out->value = value;
    return true;
}
