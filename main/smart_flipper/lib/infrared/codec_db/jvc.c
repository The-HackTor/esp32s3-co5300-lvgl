#include "jvc.h"
#include "codec_match.h"

#include <string.h>

bool ir_jvc_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    bool is_repeat = false;

    size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_JVC_BITS,
        IR_JVC_HDR_MARK, IR_JVC_HDR_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_ONE_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_ZERO_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);

    if(used == 0) {
        used = ir_match_generic(
            timings, n_timings, 0,
            IR_JVC_BITS,
            0, 0,
            IR_JVC_BIT_MARK, IR_JVC_ONE_SPACE,
            IR_JVC_BIT_MARK, IR_JVC_ZERO_SPACE,
            IR_JVC_BIT_MARK, IR_JVC_MIN_GAP,
            true,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true,
            &value);
        if(used == 0) return false;
        is_repeat = true;
    }

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_JVC;
    out->bits    = IR_JVC_BITS;
    out->value   = value;
    out->address = (uint32_t)ir_reverse_bits(value >> 8, 8);
    out->command = (uint32_t)ir_reverse_bits(value & 0xFFU, 8);
    out->repeat  = is_repeat;
    return true;
}
