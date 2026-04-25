#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ECOCLIM_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ECOCLIM_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ECOCLIM_SECTIONS      3
#define IR_ECOCLIM_BITS          56
#define IR_ECOCLIM_SHORT_BITS    15
#define IR_ECOCLIM_HDR_MARK      5730
#define IR_ECOCLIM_HDR_SPACE     1935
#define IR_ECOCLIM_BIT_MARK      440
#define IR_ECOCLIM_ONE_SPACE     1739
#define IR_ECOCLIM_ZERO_SPACE    637
#define IR_ECOCLIM_FOOTER_MARK   7820
#define IR_ECOCLIM_GAP           100000
#define IR_ECOCLIM_TOLERANCE     30
#define IR_ECOCLIM_FREQ_HZ       38000

bool ir_ecoclim_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
