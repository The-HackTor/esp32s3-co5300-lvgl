#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GICABLE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GICABLE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_GICABLE_BITS         16
#define IR_GICABLE_HDR_MARK     9000
#define IR_GICABLE_HDR_SPACE    4400
#define IR_GICABLE_BIT_MARK     550
#define IR_GICABLE_ONE_SPACE    4400
#define IR_GICABLE_ZERO_SPACE   2200
#define IR_GICABLE_RPT_SPACE    2200
#define IR_GICABLE_MIN_GAP      6450
#define IR_GICABLE_FREQ_HZ      39000

bool ir_gicable_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
