#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_NIKAI_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_NIKAI_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_NIKAI_BITS         24
#define IR_NIKAI_HDR_MARK     4000
#define IR_NIKAI_HDR_SPACE    4000
#define IR_NIKAI_BIT_MARK     500
#define IR_NIKAI_ONE_SPACE    1000
#define IR_NIKAI_ZERO_SPACE   2000
#define IR_NIKAI_MIN_GAP      8500
#define IR_NIKAI_FREQ_HZ      38000

bool ir_nikai_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
