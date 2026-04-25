#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIWA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIWA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_AIWA_RC_T501_BITS         15
#define IR_AIWA_RC_T501_PRE_BITS     26
#define IR_AIWA_RC_T501_POST_BITS    1
#define IR_AIWA_RC_T501_TOTAL_BITS   (IR_AIWA_RC_T501_BITS + IR_AIWA_RC_T501_PRE_BITS + IR_AIWA_RC_T501_POST_BITS)
#define IR_AIWA_RC_T501_PRE_DATA     0x1D8113FULL
#define IR_AIWA_RC_T501_POST_DATA    1ULL

#define IR_AIWA_NEC_HDR_MARK     8960
#define IR_AIWA_NEC_HDR_SPACE    4480
#define IR_AIWA_NEC_BIT_MARK     560
#define IR_AIWA_NEC_ONE_SPACE    1680
#define IR_AIWA_NEC_ZERO_SPACE   560
#define IR_AIWA_NEC_MIN_GAP      40000
#define IR_AIWA_FREQ_HZ          38000

bool ir_aiwa_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
