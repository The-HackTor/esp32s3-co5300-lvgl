#include "denon.h"
#include "codec_db_send.h"
#include "codec_match.h"
#include "codec_send.h"

#include <stdlib.h>
#include <string.h>

bool ir_denon_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_DENON_BITS,
        IR_DENON_HDR_MARK, IR_DENON_HDR_SPACE,
        IR_DENON_BIT_MARK, IR_DENON_ONE_SPACE,
        IR_DENON_BIT_MARK, IR_DENON_ZERO_SPACE,
        IR_DENON_BIT_MARK, 0,
        false,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_DENON;
    out->bits  = IR_DENON_BITS;
    out->value = value;
    return true;
}

esp_err_t ir_denon_send(uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq)
{
    (void)address; (void)command;
    if(!out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    const size_t cap = 2 + IR_DENON_BITS * 2 + 2;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    ir_send_buffer_t b;
    ir_send_buffer_init(&b, buf, cap, IR_DENON_FREQ_HZ, 33);
    ir_send_generic(&b,
                    IR_DENON_HDR_MARK, IR_DENON_HDR_SPACE,
                    IR_DENON_BIT_MARK, IR_DENON_ONE_SPACE,
                    IR_DENON_BIT_MARK, IR_DENON_ZERO_SPACE,
                    IR_DENON_BIT_MARK, 0,
                    raw_value & ((1ULL << IR_DENON_BITS) - 1ULL),
                    IR_DENON_BITS, false);

    if(b.overflow) { free(buf); return ESP_ERR_INVALID_SIZE; }
    *out_t    = buf;
    *out_n    = b.n;
    *out_freq = IR_DENON_FREQ_HZ;
    return ESP_OK;
}
