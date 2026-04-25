#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LG_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LG_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_LG_BITS         28
#define IR_LG32_BITS       32
#define IR_LG_HDR_MARK     8500
#define IR_LG_HDR_SPACE    4250
#define IR_LG2_HDR_MARK    3200
#define IR_LG2_HDR_SPACE   9900
#define IR_LG_BIT_MARK     550
#define IR_LG2_BIT_MARK    480
#define IR_LG_ONE_SPACE    1600
#define IR_LG_ZERO_SPACE   550
#define IR_LG_MIN_GAP      39750
#define IR_LG_FREQ_HZ      38000

bool ir_lg_decode(const uint16_t *timings, size_t n_timings,
                  ir_decode_result_t *out);
bool ir_lg2_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
