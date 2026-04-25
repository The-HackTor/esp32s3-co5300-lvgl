#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TECHNIBEL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TECHNIBEL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TECHNIBEL_AC_BITS        56
#define IR_TECHNIBEL_AC_HDR_MARK    8836
#define IR_TECHNIBEL_AC_HDR_SPACE   4380
#define IR_TECHNIBEL_AC_BIT_MARK    523
#define IR_TECHNIBEL_AC_ONE_SPACE   1696
#define IR_TECHNIBEL_AC_ZERO_SPACE  564
#define IR_TECHNIBEL_AC_GAP         100000
#define IR_TECHNIBEL_AC_FREQ_HZ     38000

bool ir_technibel_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
