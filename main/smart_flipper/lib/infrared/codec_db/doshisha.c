#include "doshisha.h"
#include "codec_match.h"

#include <string.h>

bool ir_doshisha_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_DOSHISHA_BITS,
        IR_DOSHISHA_HDR_MARK, IR_DOSHISHA_HDR_SPACE,
        IR_DOSHISHA_BIT_MARK, IR_DOSHISHA_ONE_SPACE,
        IR_DOSHISHA_BIT_MARK, IR_DOSHISHA_ZERO_SPACE,
        IR_DOSHISHA_BIT_MARK, 0,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    if((value & IR_DOSHISHA_SIGNATURE_MASK) != IR_DOSHISHA_SIGNATURE) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_DOSHISHA;
    out->bits    = IR_DOSHISHA_BITS;
    out->value   = value;
    out->command = (uint32_t)(value & IR_DOSHISHA_COMMAND_MASK);
    out->address = (uint32_t)(value & IR_DOSHISHA_CHANNEL_MASK);
    return true;
}
