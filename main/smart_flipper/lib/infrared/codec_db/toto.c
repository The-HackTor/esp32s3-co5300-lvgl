#include "toto.h"
#include "codec_match.h"

#include <string.h>

bool ir_toto_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t offset = 0;
    uint64_t prefix = 0;
    size_t used = ir_match_generic(
        timings, n_timings, offset,
        IR_TOTO_PREFIX_BITS,
        IR_TOTO_HDR_MARK, IR_TOTO_HDR_SPACE,
        IR_TOTO_BIT_MARK, IR_TOTO_ONE_SPACE,
        IR_TOTO_BIT_MARK, IR_TOTO_ZERO_SPACE,
        0, 0,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &prefix);
    if(used == 0 || prefix != IR_TOTO_PREFIX) return false;
    offset += used;

    uint64_t value = 0;
    used = ir_match_generic(
        timings, n_timings, offset,
        IR_TOTO_SHORT_BITS,
        0, 0,
        IR_TOTO_BIT_MARK, IR_TOTO_ONE_SPACE,
        IR_TOTO_BIT_MARK, IR_TOTO_ZERO_SPACE,
        IR_TOTO_BIT_MARK, IR_TOTO_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    uint8_t xor_check = 0;
    uint64_t check = value;
    for(uint16_t bits = 0; bits < IR_TOTO_SHORT_BITS; bits += 8) {
        xor_check ^= (uint8_t)(check & 0xFF);
        check >>= 8;
    }
    if(xor_check) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_TOTO;
    out->bits    = IR_TOTO_SHORT_BITS;
    out->value   = value;
    out->command = (uint32_t)(value & 0xFFFF);
    out->address = (uint32_t)((value >> 16) & 0xFFFF);
    return true;
}

bool ir_toto_long_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_TOTO_LONG;

    size_t offset = 0;
    uint64_t value = 0;
    uint8_t  data_bytes[6];
    uint16_t bytes_filled = 0;

    for(uint16_t section = 0; section < IR_TOTO_LONG_SECTIONS; section++) {
        uint64_t section_data = 0;
        bool section_ok = false;

        for(uint16_t r = 0; r < IR_TOTO_LONG_REPEATS; r++) {
            uint64_t prefix = 0;
            size_t used = ir_match_generic(
                timings, n_timings, offset,
                IR_TOTO_PREFIX_BITS,
                IR_TOTO_HDR_MARK, IR_TOTO_HDR_SPACE,
                IR_TOTO_BIT_MARK, IR_TOTO_ONE_SPACE,
                IR_TOTO_BIT_MARK, IR_TOTO_ZERO_SPACE,
                0, 0, false,
                IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
                true,
                &prefix);
            if(used == 0 || prefix != IR_TOTO_PREFIX) break;
            offset += used;

            uint64_t data = 0;
            used = ir_match_generic(
                timings, n_timings, offset,
                IR_TOTO_SHORT_BITS,
                0, 0,
                IR_TOTO_BIT_MARK, IR_TOTO_ONE_SPACE,
                IR_TOTO_BIT_MARK, IR_TOTO_ZERO_SPACE,
                IR_TOTO_BIT_MARK, IR_TOTO_GAP,
                true,
                IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
                true,
                &data);
            if(used == 0) break;
            offset += used;

            section_data = data;
            section_ok = true;
        }

        if(!section_ok) {
            if(section == 0) return false;
            break;
        }

        value = (value << IR_TOTO_SHORT_BITS) | section_data;
        for(int b = 2; b >= 0 && bytes_filled < sizeof(data_bytes); b--) {
            data_bytes[bytes_filled++] = (uint8_t)((section_data >> (b * 8)) & 0xFF);
        }
    }

    if(bytes_filled == 0) return false;

    out->bits  = (uint16_t)(bytes_filled * 8U);
    out->value = value;
    memcpy(out->state, data_bytes, bytes_filled);
    out->state_nbytes = bytes_filled;
    return true;
}
