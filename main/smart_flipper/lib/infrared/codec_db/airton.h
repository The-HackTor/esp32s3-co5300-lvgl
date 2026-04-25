#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIRTON_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIRTON_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_AIRTON_BITS        56
#define IR_AIRTON_HDR_MARK    6630
#define IR_AIRTON_HDR_SPACE   3350
#define IR_AIRTON_BIT_MARK    400
#define IR_AIRTON_ONE_SPACE   1260
#define IR_AIRTON_ZERO_SPACE  430
#define IR_AIRTON_GAP         100000
#define IR_AIRTON_FREQ_HZ     38000

bool ir_airton_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
