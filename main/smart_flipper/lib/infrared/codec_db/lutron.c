#include "lutron.h"
#include "codec_match.h"

#include <string.h>

bool ir_lutron_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    uint64_t value = 0;
    const uint16_t total_bits = IR_LUTRON_BITS + 1;
    const size_t used = ir_match_data_rle(
        timings, n_timings, 0,
        total_bits,
        IR_LUTRON_TICK_US,
        IR_DEFAULT_TOLERANCE,
        true,
        true,
        &value);
    if(used == 0) return false;

    if(((value >> IR_LUTRON_BITS) & 1ULL) != 1ULL) return false;
    value &= ((1ULL << IR_LUTRON_BITS) - 1ULL);

    out->id    = IR_CODEC_LUTRON;
    out->bits  = IR_LUTRON_BITS;
    out->value = value;
    return true;
}
