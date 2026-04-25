#include "aiwa.h"
#include "codec_match.h"

#include <string.h>

bool ir_aiwa_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_AIWA_RC_T501_TOTAL_BITS,
        IR_AIWA_NEC_HDR_MARK, IR_AIWA_NEC_HDR_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ONE_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ZERO_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    if((value & 0x1ULL) != IR_AIWA_RC_T501_POST_DATA) return false;
    value >>= IR_AIWA_RC_T501_POST_BITS;

    const uint64_t data_bits = (uint64_t)IR_AIWA_RC_T501_BITS;
    const uint64_t data_mask = (1ULL << data_bits) - 1ULL;
    const uint64_t payload   = value & data_mask;
    const uint64_t prefix    = value >> data_bits;

    if(prefix != IR_AIWA_RC_T501_PRE_DATA) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_AIWA_RC_T501;
    out->bits  = IR_AIWA_RC_T501_BITS;
    out->value = payload;
    return true;
}
