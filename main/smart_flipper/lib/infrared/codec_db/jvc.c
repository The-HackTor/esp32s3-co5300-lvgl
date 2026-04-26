#include "jvc.h"
#include "codec_db_send.h"
#include "codec_match.h"
#include "codec_send.h"

#include <stdlib.h>
#include <string.h>

bool ir_jvc_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    bool is_repeat = false;

    size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_JVC_BITS,
        IR_JVC_HDR_MARK, IR_JVC_HDR_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_ONE_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_ZERO_SPACE,
        IR_JVC_BIT_MARK, IR_JVC_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);

    if(used == 0) {
        used = ir_match_generic(
            timings, n_timings, 0,
            IR_JVC_BITS,
            0, 0,
            IR_JVC_BIT_MARK, IR_JVC_ONE_SPACE,
            IR_JVC_BIT_MARK, IR_JVC_ZERO_SPACE,
            IR_JVC_BIT_MARK, IR_JVC_MIN_GAP,
            true,
            IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
            true,
            &value);
        if(used == 0) return false;
        is_repeat = true;
    }

    memset(out, 0, sizeof(*out));
    out->id      = IR_CODEC_JVC;
    out->bits    = IR_JVC_BITS;
    out->value   = value;
    out->address = (uint32_t)ir_reverse_bits(value >> 8, 8);
    out->command = (uint32_t)ir_reverse_bits(value & 0xFFU, 8);
    out->repeat  = is_repeat;
    return true;
}

esp_err_t ir_jvc_send(uint32_t address, uint32_t command, uint64_t raw_value,
                      uint16_t **out_t, size_t *out_n, uint32_t *out_freq)
{
    if(!out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    /* Round-trip with ir_jvc_decode: addr/cmd are bit-reversed 8-bit fields. */
    uint64_t value;
    if(address || command) {
        value = ((uint64_t)ir_reverse_bits(address & 0xFF, 8) << 8)
              |  (uint64_t)ir_reverse_bits(command & 0xFF, 8);
    } else {
        value = raw_value & 0xFFFFu;
    }

    const size_t cap = 2 + IR_JVC_BITS * 2 + 2;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    ir_send_buffer_t b;
    ir_send_buffer_init(&b, buf, cap, IR_JVC_FREQ_HZ, 33);
    ir_send_generic(&b,
                    IR_JVC_HDR_MARK, IR_JVC_HDR_SPACE,
                    IR_JVC_BIT_MARK, IR_JVC_ONE_SPACE,
                    IR_JVC_BIT_MARK, IR_JVC_ZERO_SPACE,
                    IR_JVC_BIT_MARK, IR_JVC_MIN_GAP > 32767 ? 30000 : IR_JVC_MIN_GAP,
                    value, IR_JVC_BITS, true);

    if(b.overflow) { free(buf); return ESP_ERR_INVALID_SIZE; }
    *out_t    = buf;
    *out_n    = b.n;
    *out_freq = IR_JVC_FREQ_HZ;
    return ESP_OK;
}
