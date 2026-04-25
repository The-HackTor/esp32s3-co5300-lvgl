#include "midea.h"
#include "codec_match.h"

#include <string.h>

bool ir_midea_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_MIDEA_BITS,
        IR_MIDEA_HDR_MARK, IR_MIDEA_HDR_SPACE,
        IR_MIDEA_BIT_MARK, IR_MIDEA_ONE_SPACE,
        IR_MIDEA_BIT_MARK, IR_MIDEA_ZERO_SPACE,
        IR_MIDEA_BIT_MARK, IR_MIDEA_MIN_GAP,
        true,
        IR_MIDEA_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_MIDEA;
    out->bits  = IR_MIDEA_BITS;
    out->value = value;
    return true;
}

bool ir_midea24_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t longdata = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_MIDEA24_BITS * 2,
        IR_MIDEA24_HDR_MARK, IR_MIDEA24_HDR_SPACE,
        IR_MIDEA24_BIT_MARK, IR_MIDEA24_ONE_SPACE,
        IR_MIDEA24_BIT_MARK, IR_MIDEA24_ZERO_SPACE,
        IR_MIDEA24_BIT_MARK, IR_MIDEA24_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &longdata);
    if(used == 0) return false;

    uint32_t data = 0;
    for(int16_t i = IR_MIDEA24_BITS * 2; i >= 16;) {
        data <<= 8;
        i -= 8;
        const uint8_t current = (uint8_t)(longdata >> i);
        i -= 8;
        const uint8_t next = (uint8_t)(longdata >> i);
        if(current != (uint8_t)(next ^ 0xFFu)) return false;
        data |= current;
    }

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_MIDEA24;
    out->bits  = IR_MIDEA24_BITS;
    out->value = data;
    return true;
}
