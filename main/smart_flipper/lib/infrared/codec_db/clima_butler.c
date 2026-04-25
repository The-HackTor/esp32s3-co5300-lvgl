#include "clima_butler.h"
#include "codec_match.h"

#include <string.h>

bool ir_clima_butler_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_CLIMA_BUTLER_BITS,
        IR_CLIMA_BUTLER_HDR_MARK, IR_CLIMA_BUTLER_HDR_SPACE,
        IR_CLIMA_BUTLER_BIT_MARK, IR_CLIMA_BUTLER_ONE_SPACE,
        IR_CLIMA_BUTLER_BIT_MARK, IR_CLIMA_BUTLER_ZERO_SPACE,
        IR_CLIMA_BUTLER_BIT_MARK, IR_CLIMA_BUTLER_HDR_SPACE,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    size_t off = used;
    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_CLIMA_BUTLER_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    const uint16_t measured = (off < n_timings) ? timings[off] : 0;
    if(!ir_match_at_least(measured, IR_CLIMA_BUTLER_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_CLIMA_BUTLER;
    out->bits  = IR_CLIMA_BUTLER_BITS;
    out->value = value;
    return true;
}
