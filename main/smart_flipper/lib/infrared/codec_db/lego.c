#include "lego.h"
#include "codec_match.h"

#include <string.h>

bool ir_lego_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_LEGO_BITS,
        IR_LEGO_BIT_MARK, IR_LEGO_HDR_SPACE,
        IR_LEGO_BIT_MARK, IR_LEGO_ONE_SPACE,
        IR_LEGO_BIT_MARK, IR_LEGO_ZERO_SPACE,
        IR_LEGO_BIT_MARK, IR_LEGO_MIN_COMMAND_LENGTH,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    uint16_t lrc_data = (uint16_t)value;
    uint8_t lrc = 0xF;
    for(uint8_t i = 0; i < 4; i++) {
        lrc ^= (lrc_data & 0xF);
        lrc_data >>= 4;
    }
    if(lrc) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_LEGOPF;
    out->bits    = IR_LEGO_BITS;
    out->value   = value;
    out->address = (uint32_t)(((value >> (IR_LEGO_BITS - 4)) & 0x3) + 1);
    out->command = (uint32_t)((value >> 4) & 0xFF);
    return true;
}
