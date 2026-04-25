#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MIRAGE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MIRAGE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MIRAGE_STATE_BYTES   15
#define IR_MIRAGE_BITS          (IR_MIRAGE_STATE_BYTES * 8)
#define IR_MIRAGE_HDR_MARK      8360
#define IR_MIRAGE_HDR_SPACE     4248
#define IR_MIRAGE_BIT_MARK      554
#define IR_MIRAGE_ONE_SPACE     1592
#define IR_MIRAGE_ZERO_SPACE    545
#define IR_MIRAGE_GAP           100000
#define IR_MIRAGE_FREQ_HZ       38000

bool ir_mirage_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
