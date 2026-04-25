#include "zepeal.h"
#include "codec_match.h"

#include <string.h>

bool ir_zepeal_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_ZEPEAL_BITS,
        IR_ZEPEAL_HDR_MARK, IR_ZEPEAL_HDR_SPACE,
        IR_ZEPEAL_ONE_MARK,  IR_ZEPEAL_ONE_SPACE,
        IR_ZEPEAL_ZERO_MARK, IR_ZEPEAL_ZERO_SPACE,
        IR_ZEPEAL_FOOTER_MARK, IR_ZEPEAL_GAP,
        true,
        IR_ZEPEAL_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    if((value >> 8) != IR_ZEPEAL_SIGNATURE) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_ZEPEAL;
    out->bits  = IR_ZEPEAL_BITS;
    out->value = value;
    return true;
}
