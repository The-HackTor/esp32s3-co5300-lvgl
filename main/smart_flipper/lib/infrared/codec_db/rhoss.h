#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_RHOSS_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_RHOSS_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_RHOSS_STATE_LENGTH 12
#define IR_RHOSS_BITS         (IR_RHOSS_STATE_LENGTH * 8)
#define IR_RHOSS_HDR_MARK     3042
#define IR_RHOSS_HDR_SPACE    4248
#define IR_RHOSS_BIT_MARK     648
#define IR_RHOSS_ONE_SPACE    1545
#define IR_RHOSS_ZERO_SPACE   457
#define IR_RHOSS_GAP          100000
#define IR_RHOSS_FREQ_HZ      38000

bool ir_rhoss_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
