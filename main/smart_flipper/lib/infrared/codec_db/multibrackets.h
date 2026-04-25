#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MULTIBRACKETS_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MULTIBRACKETS_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MULTIBRACKETS_BITS         8
#define IR_MULTIBRACKETS_TICK_US      5000
#define IR_MULTIBRACKETS_HDR_TICKS    3
#define IR_MULTIBRACKETS_FOOTER_TICKS 6
#define IR_MULTIBRACKETS_TOLERANCE    5
#define IR_MULTIBRACKETS_FREQ_HZ      38000

bool ir_multibrackets_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
