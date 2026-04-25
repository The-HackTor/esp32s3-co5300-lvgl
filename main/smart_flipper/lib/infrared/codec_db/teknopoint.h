#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TEKNOPOINT_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TEKNOPOINT_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TEKNOPOINT_STATE_BYTES   14
#define IR_TEKNOPOINT_BITS          (IR_TEKNOPOINT_STATE_BYTES * 8)
#define IR_TEKNOPOINT_HDR_MARK      3600
#define IR_TEKNOPOINT_HDR_SPACE     1600
#define IR_TEKNOPOINT_BIT_MARK      477
#define IR_TEKNOPOINT_ONE_SPACE     1200
#define IR_TEKNOPOINT_ZERO_SPACE    530
#define IR_TEKNOPOINT_GAP           100000
#define IR_TEKNOPOINT_FREQ_HZ       38000
#define IR_TEKNOPOINT_TOLERANCE     35

bool ir_teknopoint_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
