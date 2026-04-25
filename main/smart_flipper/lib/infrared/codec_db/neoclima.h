#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_NEOCLIMA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_NEOCLIMA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_NEOCLIMA_STATE_LENGTH 12
#define IR_NEOCLIMA_BITS         (IR_NEOCLIMA_STATE_LENGTH * 8)
#define IR_NEOCLIMA_HDR_MARK     6112
#define IR_NEOCLIMA_HDR_SPACE    7391
#define IR_NEOCLIMA_BIT_MARK     537
#define IR_NEOCLIMA_ONE_SPACE    1651
#define IR_NEOCLIMA_ZERO_SPACE   571
#define IR_NEOCLIMA_GAP          100000
#define IR_NEOCLIMA_FREQ_HZ      38000

bool ir_neoclima_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
