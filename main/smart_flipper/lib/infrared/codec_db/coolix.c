#include "coolix.h"
#include "codec_match.h"

#include <string.h>

#define IR_COOLIX_EXTRA_TOLERANCE 5

bool ir_coolix_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_COOLIX;

    const uint16_t nbits = IR_COOLIX_BITS;
    if(nbits % 8 != 0) return false;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_COOLIX_HDR_MARK,
                      IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
                      IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], IR_COOLIX_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
                       IR_MARK_EXCESS)) return false;
    cursor++;

    uint64_t data = 0;
    uint64_t inverted = 0;
    for(uint16_t i = 0; i < nbits * 2; i += 8) {
        const bool flip = (i / 8) % 2;
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_COOLIX_BIT_MARK, IR_COOLIX_ONE_SPACE,
            IR_COOLIX_BIT_MARK, IR_COOLIX_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
            IR_MARK_EXCESS, true, true);
        if(!r.success) return false;
        cursor += r.used;
        if(flip) {
            inverted = (inverted << 8) | (r.data & 0xFF);
        } else {
            data = (data << 8) | (r.data & 0xFF);
        }
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_COOLIX_BIT_MARK,
                      IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
                      IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor < n_timings) {
        if(!ir_match_at_least(timings[cursor], IR_COOLIX_MIN_GAP,
                              IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
                              0)) return false;
    }

    uint64_t check = data;
    uint64_t inv = inverted;
    for(uint16_t i = 0; i < nbits; i += 8, check >>= 8, inv >>= 8) {
        if((check & 0xFF) != ((inv & 0xFF) ^ 0xFF)) return false;
    }

    out->bits  = nbits;
    out->value = data;
    return true;
}

bool ir_coolix48_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_COOLIX48;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_COOLIX48_BITS,
        IR_COOLIX_HDR_MARK, IR_COOLIX_HDR_SPACE,
        IR_COOLIX_BIT_MARK, IR_COOLIX_ONE_SPACE,
        IR_COOLIX_BIT_MARK, IR_COOLIX_ZERO_SPACE,
        IR_COOLIX_BIT_MARK, IR_COOLIX_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE + IR_COOLIX_EXTRA_TOLERANCE,
        IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    out->bits  = IR_COOLIX48_BITS;
    out->value = value;
    return true;
}
