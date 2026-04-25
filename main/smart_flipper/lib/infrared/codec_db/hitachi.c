#include "hitachi.h"
#include "codec_match.h"

#include <string.h>

static bool decode_ac_frame(const uint16_t *timings, size_t n_timings,
                            uint16_t hdr_mark, uint32_t hdr_space,
                            uint16_t bit_mark, uint32_t one_space,
                            uint32_t zero_space, uint16_t nbytes,
                            bool msb_first, uint8_t *state_out)
{
    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], hdr_mark,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor], hdr_space,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;

    for(uint16_t i = 0; i < nbytes; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            bit_mark, one_space,
            bit_mark, zero_space,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            msb_first,
            true);
        if(!r.success) return false;
        state_out[i] = (uint8_t)r.data;
        cursor += r.used;
    }

    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], bit_mark,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    cursor++;
    const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
    if(!ir_match_at_least(measured, IR_HITACHI_AC_MIN_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;
    return true;
}

bool ir_hitachi_ac1_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC1_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC1_HDR_MARK, IR_HITACHI_AC1_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC1_STATE_BYTES, true, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC1;
    out->bits = IR_HITACHI_AC1_BITS;
    memcpy(out->state, state, IR_HITACHI_AC1_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC1_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac3_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    static const uint16_t valid_sizes[] = {
        IR_HITACHI_AC3_MIN_BYTES,
        IR_HITACHI_AC3_MIN_BYTES + 2,
        IR_HITACHI_AC3_MAX_BYTES - 6,
        IR_HITACHI_AC3_MAX_BYTES - 4,
        IR_HITACHI_AC3_MAX_BYTES,
    };

    for(size_t i = 0; i < sizeof(valid_sizes) / sizeof(valid_sizes[0]); i++) {
        const uint16_t nbytes = valid_sizes[i];
        uint8_t state[IR_HITACHI_AC3_MAX_BYTES];
        if(decode_ac_frame(timings, n_timings,
                           IR_HITACHI_AC3_HDR_MARK, IR_HITACHI_AC3_HDR_SPACE,
                           IR_HITACHI_AC3_BIT_MARK,
                           IR_HITACHI_AC3_ONE_SPACE, IR_HITACHI_AC3_ZERO_SPACE,
                           nbytes, false, state)) {
            memset(out, 0, sizeof(*out));
            out->id   = IR_CODEC_HITACHI_AC3;
            out->bits = nbytes * 8;
            memcpy(out->state, state, nbytes);
            out->state_nbytes = nbytes;
            return true;
        }
    }
    return false;
}

bool ir_hitachi_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC_HDR_MARK, IR_HITACHI_AC_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC_STATE_BYTES, true, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC;
    out->bits = IR_HITACHI_AC_BITS;
    memcpy(out->state, state, IR_HITACHI_AC_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac2_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC2_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC_HDR_MARK, IR_HITACHI_AC_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC2_STATE_BYTES, true, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC2;
    out->bits = IR_HITACHI_AC2_BITS;
    memcpy(out->state, state, IR_HITACHI_AC2_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC2_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac264_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC264_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC_HDR_MARK, IR_HITACHI_AC_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC264_STATE_BYTES, true, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC264;
    out->bits = IR_HITACHI_AC264_BITS;
    memcpy(out->state, state, IR_HITACHI_AC264_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC264_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac296_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC296_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC_HDR_MARK, IR_HITACHI_AC_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC296_STATE_BYTES, false, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC296;
    out->bits = IR_HITACHI_AC296_BITS;
    memcpy(out->state, state, IR_HITACHI_AC296_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC296_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac344_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint8_t state[IR_HITACHI_AC344_STATE_BYTES];
    if(!decode_ac_frame(timings, n_timings,
                        IR_HITACHI_AC_HDR_MARK, IR_HITACHI_AC_HDR_SPACE,
                        IR_HITACHI_AC_BIT_MARK,
                        IR_HITACHI_AC_ONE_SPACE, IR_HITACHI_AC_ZERO_SPACE,
                        IR_HITACHI_AC344_STATE_BYTES, true, state))
        return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC344;
    out->bits = IR_HITACHI_AC344_BITS;
    memcpy(out->state, state, IR_HITACHI_AC344_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC344_STATE_BYTES;
    return true;
}

bool ir_hitachi_ac424_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    size_t cursor = 0;
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor++], IR_HITACHI_AC424_LDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(cursor >= n_timings) return false;
    if(!ir_match_space(timings[cursor++], IR_HITACHI_AC424_LDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;

    uint8_t state[IR_HITACHI_AC424_STATE_BYTES];
    if(!ir_match_mark(timings[cursor++], IR_HITACHI_AC424_HDR_MARK,
                      IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    if(!ir_match_space(timings[cursor++], IR_HITACHI_AC424_HDR_SPACE,
                       IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS)) return false;
    for(uint16_t i = 0; i < IR_HITACHI_AC424_STATE_BYTES; i++) {
        ir_match_result_t r = ir_match_data(
            timings, n_timings, cursor, 8,
            IR_HITACHI_AC424_BIT_MARK, IR_HITACHI_AC424_ONE_SPACE,
            IR_HITACHI_AC424_BIT_MARK, IR_HITACHI_AC424_ZERO_SPACE,
            IR_DEFAULT_TOLERANCE, 0,
            false, true);
        if(!r.success) return false;
        state[i] = (uint8_t)r.data;
        cursor += r.used;
    }
    if(cursor >= n_timings) return false;
    if(!ir_match_mark(timings[cursor], IR_HITACHI_AC424_BIT_MARK,
                      IR_DEFAULT_TOLERANCE, 0)) return false;
    cursor++;
    const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
    if(!ir_match_at_least(measured, IR_HITACHI_AC_MIN_GAP,
                          IR_DEFAULT_TOLERANCE, 0)) return false;

    memset(out, 0, sizeof(*out));
    out->id   = IR_CODEC_HITACHI_AC424;
    out->bits = IR_HITACHI_AC424_BITS;
    memcpy(out->state, state, IR_HITACHI_AC424_STATE_BYTES);
    out->state_nbytes = IR_HITACHI_AC424_STATE_BYTES;
    return true;
}
