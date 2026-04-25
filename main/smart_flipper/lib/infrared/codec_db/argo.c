#include "argo.h"
#include "codec_match.h"

#include <string.h>

static bool argo_decode_bytes(const uint16_t *timings, size_t n_timings,
                              size_t nbytes, bool with_footer,
                              ir_decode_result_t *out)
{
    size_t off = 0;

    if(off >= n_timings) return false;
    if(!ir_match_mark(timings[off++], IR_ARGO_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    if(off >= n_timings) return false;
    if(!ir_match_space(timings[off++], IR_ARGO_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, 0)) return false;

    const bool last_byte_has_space = with_footer;
    for(size_t i = 0; i < nbytes; i++) {
        const bool is_last = (i + 1 == nbytes);
        const bool expect_last_space = is_last ? last_byte_has_space : true;
        ir_match_result_t r = ir_match_data(
            timings, n_timings, off, 8,
            IR_ARGO_BIT_MARK, IR_ARGO_ONE_SPACE,
            IR_ARGO_BIT_MARK, IR_ARGO_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, 0,
            false, expect_last_space);
        if(!r.success) return false;
        out->state[i] = (uint8_t)(r.data & 0xFFu);
        off += r.used;
    }

    if(with_footer) {
        if(off >= n_timings) return false;
        if(!ir_match_mark(timings[off++], IR_ARGO_BIT_MARK,
                          IR_DEFAULT_TOLERANCE, 0)) return false;
        const uint16_t measured = (off < n_timings) ? timings[off] : 0;
        if(!ir_match_at_least(measured, IR_ARGO_GAP,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    }

    out->state_nbytes = (uint16_t)nbytes;
    out->bits         = (uint16_t)(nbytes * 8);
    out->id           = IR_CODEC_UNKNOWN;
    return true;
}

bool ir_argo_wrem2_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    return argo_decode_bytes(timings, n_timings, IR_ARGO_WREM2_BYTES,
                             false, out);
}

bool ir_argo_wrem3_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    static const size_t variants[] = {
        IR_ARGO_WREM3_AC_BYTES,
        IR_ARGO_WREM3_CFG_BYTES,
        IR_ARGO_WREM3_IFEEL_BYTES,
        IR_ARGO_WREM3_TIMER_BYTES,
    };
    for(size_t v = 0; v < sizeof(variants) / sizeof(variants[0]); v++) {
        memset(out, 0, sizeof(*out));
        if(argo_decode_bytes(timings, n_timings, variants[v], true, out)) {
            return true;
        }
    }
    return false;
}
