#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_RCMM_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_RCMM_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_RCMM_BITS         24
#define IR_RCMM_HDR_MARK     416
#define IR_RCMM_HDR_SPACE    277
#define IR_RCMM_BIT_MARK     166
#define IR_RCMM_ZERO_SPACE   277
#define IR_RCMM_ONE_SPACE    444
#define IR_RCMM_TWO_SPACE    611
#define IR_RCMM_THREE_SPACE  777
#define IR_RCMM_MIN_GAP      3360
#define IR_RCMM_TOLERANCE    10
#define IR_RCMM_FREQ_HZ      36000

bool ir_rcmm_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
