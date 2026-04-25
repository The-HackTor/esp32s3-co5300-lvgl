#include "ecoclim.h"
#include "codec_match.h"

#include <string.h>

static bool ecoclim_try(const uint16_t *timings, size_t n_timings,
                        uint16_t nbits, ir_decode_result_t *out)
{
    size_t off = 0;
    uint64_t first = 0;

    for(uint8_t section = 0; section < IR_ECOCLIM_SECTIONS; section++) {
        uint64_t data = 0;
        const size_t used = ir_match_generic(
            timings, n_timings, off, nbits,
            IR_ECOCLIM_HDR_MARK, IR_ECOCLIM_HDR_SPACE,
            IR_ECOCLIM_BIT_MARK, IR_ECOCLIM_ONE_SPACE,
            IR_ECOCLIM_BIT_MARK, IR_ECOCLIM_ZERO_SPACE,
            0, 0,
            false,
            IR_ECOCLIM_TOLERANCE, 0,
            true,
            &data);
        if(used == 0) return false;
        off += used;
        if(section == 0) first = data;
        else if(data != first) return false;
    }

    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_ECOCLIM_FOOTER_MARK,
                      IR_ECOCLIM_TOLERANCE, 0)) return false;
    const uint16_t measured = (off < n_timings) ? timings[off] : 0;
    if(!ir_match_at_least(measured, IR_ECOCLIM_GAP,
                          IR_ECOCLIM_TOLERANCE, 0)) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_UNKNOWN;
    out->bits  = nbits;
    out->value = first;
    return true;
}

bool ir_ecoclim_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    if(ecoclim_try(timings, n_timings, IR_ECOCLIM_BITS, out)) return true;
    if(ecoclim_try(timings, n_timings, IR_ECOCLIM_SHORT_BITS, out)) return true;
    return false;
}
