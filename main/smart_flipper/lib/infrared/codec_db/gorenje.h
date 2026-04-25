#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GORENJE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GORENJE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_GORENJE_BITS         8
#define IR_GORENJE_BIT_MARK     1300
#define IR_GORENJE_ONE_SPACE    5700
#define IR_GORENJE_ZERO_SPACE   1700
#define IR_GORENJE_MIN_GAP      100000
#define IR_GORENJE_FREQ_HZ      38000
#define IR_GORENJE_TOLERANCE    7

bool ir_gorenje_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
