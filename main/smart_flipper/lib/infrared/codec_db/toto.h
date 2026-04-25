#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TOTO_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TOTO_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TOTO_SHORT_BITS    24
#define IR_TOTO_HDR_MARK      6197
#define IR_TOTO_HDR_SPACE     2754
#define IR_TOTO_BIT_MARK      600
#define IR_TOTO_ONE_SPACE     1634
#define IR_TOTO_ZERO_SPACE    516
#define IR_TOTO_GAP           38000
#define IR_TOTO_PREFIX        0x0802
#define IR_TOTO_PREFIX_BITS   15
#define IR_TOTO_FREQ_HZ       38000

#define IR_TOTO_LONG_BITS     48
#define IR_TOTO_LONG_SECTIONS 2
#define IR_TOTO_LONG_REPEATS  3

bool ir_toto_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

bool ir_toto_long_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
