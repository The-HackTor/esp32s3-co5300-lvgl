#include "aiwa.h"
#include "codec_db_send.h"
#include "codec_match.h"
#include "codec_send.h"

#include <stdlib.h>
#include <string.h>

bool ir_aiwa_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out)
{
    if(!timings || !out) return false;

    uint64_t value = 0;
    const size_t used = ir_match_generic(
        timings, n_timings, 0,
        IR_AIWA_RC_T501_TOTAL_BITS,
        IR_AIWA_NEC_HDR_MARK, IR_AIWA_NEC_HDR_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ONE_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ZERO_SPACE,
        IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_MIN_GAP,
        true,
        IR_DEFAULT_TOLERANCE, IR_MARK_EXCESS,
        true,
        &value);
    if(used == 0) return false;

    if((value & 0x1ULL) != IR_AIWA_RC_T501_POST_DATA) return false;
    value >>= IR_AIWA_RC_T501_POST_BITS;

    const uint64_t data_bits = (uint64_t)IR_AIWA_RC_T501_BITS;
    const uint64_t data_mask = (1ULL << data_bits) - 1ULL;
    const uint64_t payload   = value & data_mask;
    const uint64_t prefix    = value >> data_bits;

    if(prefix != IR_AIWA_RC_T501_PRE_DATA) return false;

    memset(out, 0, sizeof(*out));
    out->id    = IR_CODEC_AIWA_RC_T501;
    out->bits  = IR_AIWA_RC_T501_BITS;
    out->value = payload;
    return true;
}

esp_err_t ir_aiwa_send(uint32_t address, uint32_t command, uint64_t raw_value,
                       uint16_t **out_t, size_t *out_n, uint32_t *out_freq)
{
    (void)address; (void)command;
    if(!out_t || !out_n || !out_freq) return ESP_ERR_INVALID_ARG;

    const uint64_t data = raw_value & ((1ULL << IR_AIWA_RC_T501_BITS) - 1ULL);
    const uint64_t wire = (IR_AIWA_RC_T501_PRE_DATA
                           << (IR_AIWA_RC_T501_BITS + IR_AIWA_RC_T501_POST_BITS))
                        | (data << IR_AIWA_RC_T501_POST_BITS)
                        | IR_AIWA_RC_T501_POST_DATA;

    const size_t cap = 2 + IR_AIWA_RC_T501_TOTAL_BITS * 2 + 2;
    uint16_t *buf = malloc(cap * sizeof(uint16_t));
    if(!buf) return ESP_ERR_NO_MEM;

    ir_send_buffer_t b;
    ir_send_buffer_init(&b, buf, cap, IR_AIWA_FREQ_HZ, 33);
    ir_send_generic(&b,
                    IR_AIWA_NEC_HDR_MARK, IR_AIWA_NEC_HDR_SPACE,
                    IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ONE_SPACE,
                    IR_AIWA_NEC_BIT_MARK, IR_AIWA_NEC_ZERO_SPACE,
                    IR_AIWA_NEC_BIT_MARK, 30000,   /* upstream 40000 > uint16_t cap */
                    wire, IR_AIWA_RC_T501_TOTAL_BITS, true);

    if(b.overflow) { free(buf); return ESP_ERR_INVALID_SIZE; }
    *out_t    = buf;
    *out_n    = b.n;
    *out_freq = IR_AIWA_FREQ_HZ;
    return ESP_OK;
}
