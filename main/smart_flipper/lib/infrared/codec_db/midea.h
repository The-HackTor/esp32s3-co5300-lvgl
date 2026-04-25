#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MIDEA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MIDEA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MIDEA_BITS         48
#define IR_MIDEA_HDR_MARK     4480
#define IR_MIDEA_HDR_SPACE    4480
#define IR_MIDEA_BIT_MARK     560
#define IR_MIDEA_ONE_SPACE    1680
#define IR_MIDEA_ZERO_SPACE   560
#define IR_MIDEA_MIN_GAP      5600
#define IR_MIDEA_FREQ_HZ      38000
#define IR_MIDEA_TOLERANCE    30

#define IR_MIDEA24_BITS       24
#define IR_MIDEA24_HDR_MARK   8960
#define IR_MIDEA24_HDR_SPACE  4480
#define IR_MIDEA24_BIT_MARK   560
#define IR_MIDEA24_ONE_SPACE  1680
#define IR_MIDEA24_ZERO_SPACE 560
#define IR_MIDEA24_MIN_GAP    13000

bool ir_midea_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);
bool ir_midea24_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
