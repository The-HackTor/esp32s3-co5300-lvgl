#include "multibrackets.h"
#include "codec_match.h"

#include <string.h>

bool ir_multibrackets_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    if(n_timings < 1) return false;

    const uint32_t hdr_us = (uint32_t)IR_MULTIBRACKETS_TICK_US * IR_MULTIBRACKETS_HDR_TICKS;
    if(!ir_match_at_least(timings[0], hdr_us, IR_MULTIBRACKETS_TOLERANCE, 0)) return false;

    int32_t remaining = (int32_t)timings[0] - (int32_t)hdr_us;
    size_t cursor = 0;
    bool polarity = true;
    uint16_t bits = 0;
    uint64_t data = 0;

    while(bits < IR_MULTIBRACKETS_BITS && cursor < n_timings) {
        if(remaining <= 0) {
            polarity = !polarity;
            cursor++;
            if(cursor >= n_timings) break;
            remaining = (int32_t)timings[cursor];
            continue;
        }
        if(ir_match_at_least((uint16_t)remaining, IR_MULTIBRACKETS_TICK_US,
                             IR_MULTIBRACKETS_TOLERANCE, 0)) {
            data = (data << 1) | (polarity ? 1ULL : 0ULL);
            bits++;
        }
        remaining -= IR_MULTIBRACKETS_TICK_US;
    }

    if(bits != IR_MULTIBRACKETS_BITS) return false;

    if(remaining > 0) {
        if(!ir_match_at_least((uint16_t)remaining,
                              (uint32_t)IR_MULTIBRACKETS_TICK_US * IR_MULTIBRACKETS_FOOTER_TICKS,
                              IR_MULTIBRACKETS_TOLERANCE, 0)) return false;
    } else {
        cursor++;
        if(cursor < n_timings) {
            if(!ir_match_at_least(timings[cursor],
                                  (uint32_t)IR_MULTIBRACKETS_TICK_US * IR_MULTIBRACKETS_FOOTER_TICKS,
                                  IR_MULTIBRACKETS_TOLERANCE, 0)) return false;
        }
    }

    out->id    = IR_CODEC_MULTIBRACKETS;
    out->bits  = IR_MULTIBRACKETS_BITS;
    out->value = data;
    return true;
}
