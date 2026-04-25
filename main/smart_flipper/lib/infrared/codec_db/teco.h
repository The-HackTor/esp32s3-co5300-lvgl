#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TECO_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TECO_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TECO_BITS         35
#define IR_TECO_HDR_MARK     9000
#define IR_TECO_HDR_SPACE    4440
#define IR_TECO_BIT_MARK     620
#define IR_TECO_ONE_SPACE    1650
#define IR_TECO_ZERO_SPACE   580
#define IR_TECO_GAP          100000
#define IR_TECO_FREQ_HZ      38000

bool ir_teco_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
