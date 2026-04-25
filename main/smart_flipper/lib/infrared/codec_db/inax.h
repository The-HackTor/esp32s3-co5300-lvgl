#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_INAX_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_INAX_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_INAX_BITS         24
#define IR_INAX_HDR_MARK     9000
#define IR_INAX_HDR_SPACE    4500
#define IR_INAX_BIT_MARK     560
#define IR_INAX_ONE_SPACE    1675
#define IR_INAX_ZERO_SPACE   560
#define IR_INAX_MIN_GAP      40000
#define IR_INAX_FREQ_HZ      38000

bool ir_inax_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
