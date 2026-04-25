#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MAGIQUEST_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MAGIQUEST_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MAGIQUEST_BITS         56
#define IR_MAGIQUEST_TOTAL_US     1150
#define IR_MAGIQUEST_ZERO_PCT_MAX 30
#define IR_MAGIQUEST_ONE_PCT_MIN  38
#define IR_MAGIQUEST_FREQ_HZ      36000

bool ir_magiquest_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
