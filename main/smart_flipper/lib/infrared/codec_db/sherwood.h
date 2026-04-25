#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SHERWOOD_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SHERWOOD_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_SHERWOOD_BITS        32
#define IR_SHERWOOD_HDR_MARK    9000
#define IR_SHERWOOD_HDR_SPACE   4500
#define IR_SHERWOOD_BIT_MARK    560
#define IR_SHERWOOD_ONE_SPACE   1690
#define IR_SHERWOOD_ZERO_SPACE  560
#define IR_SHERWOOD_MIN_GAP     40000
#define IR_SHERWOOD_MIN_REPEAT  1
#define IR_SHERWOOD_FREQ_HZ     38000

bool ir_sherwood_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
