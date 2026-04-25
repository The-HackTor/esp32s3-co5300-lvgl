#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WHIRLPOOL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_WHIRLPOOL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_WHIRLPOOL_AC_STATE_BYTES 21
#define IR_WHIRLPOOL_AC_BITS        (IR_WHIRLPOOL_AC_STATE_BYTES * 8)
#define IR_WHIRLPOOL_AC_HDR_MARK    8950
#define IR_WHIRLPOOL_AC_HDR_SPACE   4484
#define IR_WHIRLPOOL_AC_BIT_MARK    597
#define IR_WHIRLPOOL_AC_ONE_SPACE   1649
#define IR_WHIRLPOOL_AC_ZERO_SPACE  533
#define IR_WHIRLPOOL_AC_GAP         7920
#define IR_WHIRLPOOL_AC_MIN_GAP     100000

bool ir_whirlpool_ac_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
