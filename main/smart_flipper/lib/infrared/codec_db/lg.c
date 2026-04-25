#include "lg.h"
#include "codec_match.h"

#include <string.h>

static bool lg_decode_variant(const uint16_t *timings, size_t n_timings,
                              uint16_t hdr_mark, uint32_t hdr_space,
                              uint16_t bit_mark,
                              ir_decode_result_t *out)
{
    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_LG_BITS,
        hdr_mark, hdr_space,
        bit_mark, IR_LG_ONE_SPACE,
        bit_mark, IR_LG_ZERO_SPACE,
        bit_mark, IR_LG_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, 0,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_UNKNOWN;
    out->bits    = IR_LG_BITS;
    out->value   = value;
    out->command = (uint32_t)((value >> 4) & 0xFFFFu);
    out->address = (uint32_t)(value >> 20);
    return true;
}

bool ir_lg_decode(const uint16_t *timings, size_t n_timings,
                  ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    return lg_decode_variant(timings, n_timings,
                             IR_LG_HDR_MARK, IR_LG_HDR_SPACE,
                             IR_LG_BIT_MARK, out);
}

bool ir_lg2_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    return lg_decode_variant(timings, n_timings,
                             IR_LG2_HDR_MARK, IR_LG2_HDR_SPACE,
                             IR_LG2_BIT_MARK, out);
}
