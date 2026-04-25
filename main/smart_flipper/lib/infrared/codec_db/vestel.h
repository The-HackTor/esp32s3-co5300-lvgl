#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_VESTEL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_VESTEL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_VESTEL_AC_BITS       56
#define IR_VESTEL_AC_HDR_MARK   3110
#define IR_VESTEL_AC_HDR_SPACE  9066
#define IR_VESTEL_AC_BIT_MARK   520
#define IR_VESTEL_AC_ONE_SPACE  1535
#define IR_VESTEL_AC_ZERO_SPACE 480
#define IR_VESTEL_AC_TOLERANCE  30

bool ir_vestel_ac_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
