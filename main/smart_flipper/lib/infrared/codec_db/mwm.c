#include "mwm.h"
#include "codec_match.h"

#include <string.h>

static uint16_t mwm_cells(uint16_t measured_us)
{
    if(measured_us == 0) return 0;
    uint32_t n = ((uint32_t)measured_us + IR_MWM_TICK_US / 2) / IR_MWM_TICK_US;
    if(n == 0) n = 1;
    if(n > 9) n = 9;
    const uint32_t expected = n * IR_MWM_TICK_US;
    const uint32_t diff = (measured_us > expected) ? (measured_us - expected)
                                                   : (expected - measured_us);
    if(diff > IR_MWM_DELTA_US) return 0;
    return (uint16_t)n;
}

bool ir_mwm_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;
    memset(out, 0, sizeof(*out));
    out->id = IR_CODEC_MWM;

    if(n_timings < 6) return false;

    uint8_t  bytes[IR_MWM_MAX_BYTES];
    uint16_t byte_count = 0;
    uint16_t frame_pos = 0;
    uint8_t  current = 0;
    uint8_t  data_bits = 0;

    for(size_t i = 0; i < n_timings && byte_count < IR_MWM_MAX_BYTES; i++) {
        const uint8_t  level = (i & 1U) ? 1U : 0U;
        const uint16_t cells = mwm_cells(timings[i]);
        if(cells == 0) {
            if(level == 1U && (uint32_t)timings[i] >= IR_MWM_MIN_GAP) break;
            return false;
        }

        for(uint16_t c = 0; c < cells; c++) {
            const uint16_t slot = frame_pos % IR_MWM_BITS_PER_FRAME;
            if(slot == 0) {
                if(level != 0U) goto finalize;
                current = 0;
                data_bits = 0;
            } else if(slot == 9) {
                if(level != 1U) return false;
                if(byte_count < IR_MWM_MAX_BYTES) {
                    bytes[byte_count++] = current;
                }
            } else {
                if(level == 1U) {
                    current |= (uint8_t)(1U << data_bits);
                }
                data_bits++;
            }
            frame_pos++;
        }
    }

finalize:
    if(byte_count == 0) return false;

    memcpy(out->state, bytes, byte_count);
    out->state_nbytes = byte_count;
    out->bits = (uint16_t)(byte_count * 8U);
    return true;
}
