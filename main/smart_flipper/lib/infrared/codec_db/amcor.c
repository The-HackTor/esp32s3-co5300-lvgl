#include "amcor.h"
#include "codec_match.h"

#include <string.h>

bool ir_amcor_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    memset(out, 0, sizeof(*out));

    uint64_t chunk = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_AMCOR_BITS,
        IR_AMCOR_HDR_MARK, IR_AMCOR_HDR_SPACE,
        IR_AMCOR_ONE_MARK, IR_AMCOR_ONE_SPACE,
        IR_AMCOR_ZERO_MARK, IR_AMCOR_ZERO_SPACE,
        IR_AMCOR_FOOTER_MARK, IR_AMCOR_GAP,
        true,
        IR_AMCOR_TOLERANCE, 0,
        false,
        &chunk);
    if(used == 0) return false;

    for(size_t i = 0; i < IR_AMCOR_STATE_LENGTH; i++) {
        out->state[i] = (uint8_t)((chunk >> (i * 8)) & 0xFFu);
    }
    out->id           = IR_CODEC_AMCOR;
    out->bits         = IR_AMCOR_BITS;
    out->state_nbytes = IR_AMCOR_STATE_LENGTH;
    return true;
}
