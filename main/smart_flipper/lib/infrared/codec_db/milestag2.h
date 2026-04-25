#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MILESTAG2_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MILESTAG2_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MILESTAG2_SHOT_BITS     14
#define IR_MILESTAG2_MSG_BITS      24
#define IR_MILESTAG2_HDR_MARK      2400
#define IR_MILESTAG2_SPACE         600
#define IR_MILESTAG2_ONE_MARK      1200
#define IR_MILESTAG2_ZERO_MARK     600
#define IR_MILESTAG2_RPT_LENGTH    32000
#define IR_MILESTAG2_FREQ_HZ       38000
#define IR_MILESTAG2_MSG_TERMINATOR 0xE8

bool ir_milestag2_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
