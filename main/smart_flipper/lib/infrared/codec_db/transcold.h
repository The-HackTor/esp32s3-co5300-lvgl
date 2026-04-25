#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TRANSCOLD_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TRANSCOLD_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TRANSCOLD_BITS        24
#define IR_TRANSCOLD_HDR_MARK    5944
#define IR_TRANSCOLD_HDR_SPACE   7563
#define IR_TRANSCOLD_BIT_MARK    555
#define IR_TRANSCOLD_ONE_SPACE   3556
#define IR_TRANSCOLD_ZERO_SPACE  1526
#define IR_TRANSCOLD_GAP         100000
#define IR_TRANSCOLD_FREQ_HZ     38000

bool ir_transcold_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
