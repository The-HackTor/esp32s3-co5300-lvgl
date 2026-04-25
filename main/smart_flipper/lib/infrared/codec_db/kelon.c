#include "kelon.h"
#include "codec_match.h"

#include <string.h>

bool ir_kelon_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_KELON_BITS,
        IR_KELON_HDR_MARK, IR_KELON_HDR_SPACE,
        IR_KELON_BIT_MARK, IR_KELON_ONE_SPACE,
        IR_KELON_BIT_MARK, IR_KELON_ZERO_SPACE,
        IR_KELON_BIT_MARK, IR_KELON_GAP,
        true,
        IR_DEFAULT_TOLERANCE, 0,
        false,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_KELON;
    out->bits  = IR_KELON_BITS;
    out->value = value;
    return true;
}

static bool kelon168_section(const uint16_t *timings, size_t n_timings,
                             size_t *cursor,
                             uint16_t hdr_mark, uint32_t hdr_space,
                             uint32_t footer_space, bool at_least_footer,
                             uint8_t *state_out, uint16_t nbytes)
{
    size_t c = *cursor;
    if(hdr_mark) {
        if(c >= n_timings) return false;
        if(!ir_match_mark(timings[c], hdr_mark,
                          IR_DEFAULT_TOLERANCE, 0)) return false;
        c++;
        if(c >= n_timings) return false;
        if(!ir_match_space(timings[c], hdr_space,
                           IR_DEFAULT_TOLERANCE, 0)) return false;
        c++;
    }
    for(uint16_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, c, 8,
            IR_KELON_BIT_MARK, IR_KELON_ONE_SPACE,
            IR_KELON_BIT_MARK, IR_KELON_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, 0,
            false, true);
        if(!r.success) return false;
        state_out[i] = (uint8_t)r.data;
        c += r.used;
    }
    if(c >= n_timings) return false;
    if(!ir_match_mark(timings[c], IR_KELON_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    c++;
    const uint16_t measured = (c < n_timings) ? timings[c] : 0;
    if(at_least_footer) {
        if(!ir_match_at_least(measured, footer_space,
                              IR_DEFAULT_TOLERANCE, 0)) return false;
    } else {
        if(!ir_match_space(measured, footer_space,
                           IR_DEFAULT_TOLERANCE, 0)) return false;
    }
    if(c < n_timings) c++;
    *cursor = c;
    return true;
}

bool ir_kelon168_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));

    size_t cursor = 0;
    if(!kelon168_section(timings, n_timings, &cursor,
                         IR_KELON_HDR_MARK, IR_KELON_HDR_SPACE,
                         IR_KELON168_FOOTER_SPACE, false,
                         out->state, IR_KELON168_SECTION1_BYTES)) return false;
    if(!kelon168_section(timings, n_timings, &cursor,
                         0, 0,
                         IR_KELON168_FOOTER_SPACE, false,
                         out->state + IR_KELON168_SECTION1_BYTES,
                         IR_KELON168_SECTION2_BYTES)) return false;
    if(!kelon168_section(timings, n_timings, &cursor,
                         0, 0,
                         IR_KELON_GAP, true,
                         out->state + IR_KELON168_SECTION1_BYTES +
                                      IR_KELON168_SECTION2_BYTES,
                         IR_KELON168_SECTION3_BYTES)) return false;

    out->id           = IR_CODEC_KELON168;
    out->bits         = IR_KELON168_BITS;
    out->state_nbytes = IR_KELON168_STATE_BYTES;
    return true;
}
