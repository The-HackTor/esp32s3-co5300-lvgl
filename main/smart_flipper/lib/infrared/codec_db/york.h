#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_YORK_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_YORK_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_YORK_STATE_BYTES 17
#define IR_YORK_BITS        (IR_YORK_STATE_BYTES * 8)
#define IR_YORK_HDR_MARK    4887
#define IR_YORK_HDR_SPACE   2267
#define IR_YORK_BIT_MARK    612
#define IR_YORK_ONE_SPACE   1778
#define IR_YORK_ZERO_SPACE  579
#define IR_YORK_GAP         100000

bool ir_york_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
