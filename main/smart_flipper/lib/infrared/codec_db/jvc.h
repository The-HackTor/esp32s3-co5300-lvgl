#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_JVC_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_JVC_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_JVC_BITS         16
#define IR_JVC_HDR_MARK     8400
#define IR_JVC_HDR_SPACE    4200
#define IR_JVC_BIT_MARK     525
#define IR_JVC_ONE_SPACE    1725
#define IR_JVC_ZERO_SPACE   525
#define IR_JVC_MIN_GAP      37950
#define IR_JVC_FREQ_HZ      38000

bool ir_jvc_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
