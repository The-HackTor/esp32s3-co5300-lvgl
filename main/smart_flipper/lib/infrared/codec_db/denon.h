#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DENON_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DENON_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DENON_BITS         15
#define IR_DENON_HDR_MARK     263
#define IR_DENON_HDR_SPACE    789
#define IR_DENON_BIT_MARK     263
#define IR_DENON_ONE_SPACE    1841
#define IR_DENON_ZERO_SPACE   789
#define IR_DENON_FREQ_HZ      38000

bool ir_denon_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
