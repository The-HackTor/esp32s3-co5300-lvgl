#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MITSUBISHI_HEAVY_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MITSUBISHI_HEAVY_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MITSUBISHI_HEAVY_HDR_MARK     3140
#define IR_MITSUBISHI_HEAVY_HDR_SPACE    1630
#define IR_MITSUBISHI_HEAVY_BIT_MARK     370
#define IR_MITSUBISHI_HEAVY_ONE_SPACE    420
#define IR_MITSUBISHI_HEAVY_ZERO_SPACE   1220
#define IR_MITSUBISHI_HEAVY_GAP          100000

#define IR_MITSUBISHI_HEAVY_88_STATE_BYTES  11
#define IR_MITSUBISHI_HEAVY_88_BITS         (IR_MITSUBISHI_HEAVY_88_STATE_BYTES * 8)
#define IR_MITSUBISHI_HEAVY_152_STATE_BYTES 19
#define IR_MITSUBISHI_HEAVY_152_BITS        (IR_MITSUBISHI_HEAVY_152_STATE_BYTES * 8)

bool ir_mitsubishi_heavy_88_decode(const uint16_t *timings, size_t n_timings,
                                   ir_decode_result_t *out);
bool ir_mitsubishi_heavy_152_decode(const uint16_t *timings, size_t n_timings,
                                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
