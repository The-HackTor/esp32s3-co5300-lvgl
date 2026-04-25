#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WOWWEE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WOWWEE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_WOWWEE_BITS         11
#define IR_WOWWEE_HDR_MARK     6684
#define IR_WOWWEE_HDR_SPACE    723
#define IR_WOWWEE_BIT_MARK     912
#define IR_WOWWEE_ONE_SPACE    3259
#define IR_WOWWEE_ZERO_SPACE   723
#define IR_WOWWEE_MSG_GAP      100000
#define IR_WOWWEE_FREQ_HZ      38000

bool ir_wowwee_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
