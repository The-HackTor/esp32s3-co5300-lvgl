#include "lasertag.h"
#include "codec_match.h"

#include <string.h>

bool ir_lasertag_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_manchester(
        timings, n_timings, 0,
        IR_LASERTAG_BITS,
        0, 0,
        IR_LASERTAG_HALF_PERIOD,
        0, 0,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_LASERTAG;
    out->bits    = IR_LASERTAG_BITS;
    out->value   = value;
    out->address = (uint32_t)(value & 0xF);
    out->command = (uint32_t)(value >> 4);
    return true;
}
