#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BOSE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BOSE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_BOSE_BITS        16
#define IR_BOSE_HDR_MARK    1100
#define IR_BOSE_HDR_SPACE   1350
#define IR_BOSE_BIT_MARK    555
#define IR_BOSE_ONE_SPACE   1435
#define IR_BOSE_ZERO_SPACE  500
#define IR_BOSE_GAP_MIN     20000
#define IR_BOSE_FREQ_HZ     38000

bool ir_bose_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
