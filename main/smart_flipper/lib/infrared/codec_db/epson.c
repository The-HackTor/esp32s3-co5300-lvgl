#include "epson.h"
#include "codec_match.h"

#include <string.h>

bool ir_epson_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_EPSON_BITS,
        IR_EPSON_HDR_MARK, IR_EPSON_HDR_SPACE,
        IR_EPSON_BIT_MARK, IR_EPSON_ONE_SPACE,
        IR_EPSON_BIT_MARK, IR_EPSON_ZERO_SPACE,
        IR_EPSON_BIT_MARK, IR_EPSON_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    const uint8_t command = (value & 0xFF00) >> 8;
    if((command ^ 0xFF) != (value & 0xFF)) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_EPSON;
    out->bits    = IR_EPSON_BITS;
    out->value   = value;
    out->command = (uint32_t)ir_reverse_bits(command, 8);

    const uint8_t address = (value & 0xFF000000U) >> 24;
    const uint8_t address_inverted = (value & 0x00FF0000U) >> 16;
    if(address == (uint8_t)(address_inverted ^ 0xFF)) {
        out->address = (uint32_t)ir_reverse_bits(address, 8);
    } else {
        out->address = (uint32_t)ir_reverse_bits((value >> 16) & 0xFFFFU, 16);
    }
    return true;
}
