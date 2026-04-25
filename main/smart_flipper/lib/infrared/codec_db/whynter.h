#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WHYNTER_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WHYNTER_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_WHYNTER_BITS         32
#define IR_WHYNTER_HDR_MARK     2850
#define IR_WHYNTER_HDR_SPACE    2850
#define IR_WHYNTER_BIT_MARK     750
#define IR_WHYNTER_ONE_SPACE    2150
#define IR_WHYNTER_ZERO_SPACE   750
#define IR_WHYNTER_MIN_GAP      12200
#define IR_WHYNTER_FREQ_HZ      38000

bool ir_whynter_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
