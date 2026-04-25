#include "transcold.h"
#include "codec_match.h"

#include <string.h>

bool ir_transcold_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    const uint16_t nbits = IR_TRANSCOLD_BITS;
    if(nbits % 8 != 0) return false;

    size_t off = 0;
    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_TRANSCOLD_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_space(timings[off++], IR_TRANSCOLD_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint64_t data = 0;
    uint64_t inverted = 0;
    for(uint16_t i = 0; i < nbits * 2; i++) {
        const bool flip = ((i / 8) % 2) != 0;
        if(off >= n_timings) return false;
        if(!ir_match_mark(timings[off++], IR_TRANSCOLD_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
        if(off >= n_timings) return false;
        const uint16_t s = timings[off++];
        uint64_t *dst = flip ? &inverted : &data;
        if(ir_match_space(s, IR_TRANSCOLD_ONE_SPACE,
                          IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) {
            *dst = (*dst << 1) | 1ULL;
        } else if(ir_match_space(s, IR_TRANSCOLD_ZERO_SPACE,
                                 IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) {
            *dst = *dst << 1;
        } else {
            return false;
        }
    }

    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_TRANSCOLD_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_space(timings[off++], IR_TRANSCOLD_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_TRANSCOLD_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    const uint16_t measured = (off < n_timings) ? timings[off] : 0;
    if(!ir_match_at_least(measured, IR_TRANSCOLD_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    const uint64_t mask = (nbits >= 64) ? ~0ULL : ((1ULL << nbits) - 1ULL);
    if((inverted & mask) != ((~data) & mask)) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_TRANSCOLD;
    out->bits  = nbits;
    out->value = data;
    return true;
}
