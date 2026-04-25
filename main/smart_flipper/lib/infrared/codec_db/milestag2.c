#include "milestag2.h"
#include "codec_match.h"

#include <string.h>

static bool try_decode(const uint16_t *timings, size_t n_timings,
                       uint16_t nbits, uint64_t *out_value)
{
    return ir_match_generic(
        timings, n_timings, 0,
        nbits,
        IR_MILESTAG2_HDR_MARK, IR_MILESTAG2_SPACE,
        IR_MILESTAG2_ONE_MARK, IR_MILESTAG2_SPACE,
        IR_MILESTAG2_ZERO_MARK, IR_MILESTAG2_SPACE,
        0, IR_MILESTAG2_RPT_LENGTH,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        out_value) != 0;
}

bool ir_milestag2_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    uint16_t nbits = 0;

    if(try_decode(timings, n_timings, IR_MILESTAG2_MSG_BITS, &value)) {
        const uint32_t msg_mask = 1u << (IR_MILESTAG2_MSG_BITS - 1);
        if((value & msg_mask) &&
           ((value & 0xFF) == IR_MILESTAG2_MSG_TERMINATOR)) {
            nbits = IR_MILESTAG2_MSG_BITS;
        }
    }
    if(!nbits && try_decode(timings, n_timings, IR_MILESTAG2_SHOT_BITS, &value)) {
        const uint16_t shot_mask = 1u << (IR_MILESTAG2_SHOT_BITS - 1);
        if(!(value & shot_mask)) {
            nbits = IR_MILESTAG2_SHOT_BITS;
        }
    }
    if(!nbits) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_MILESTAG2;
    out->bits  = nbits;
    out->value = value;
    if(nbits == IR_MILESTAG2_SHOT_BITS) {
        out->command = (uint32_t)(value & 0x3F);
        out->address = (uint32_t)(value >> 6);
    } else {
        out->command = (uint32_t)((value >> 8) & 0xFF);
        out->address = (uint32_t)((value >> 16) & 0x7F);
    }
    return true;
}
