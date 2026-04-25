#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DELONGHI_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DELONGHI_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DELONGHI_BITS        64
#define IR_DELONGHI_HDR_MARK    8984
#define IR_DELONGHI_HDR_SPACE   4200
#define IR_DELONGHI_BIT_MARK    572
#define IR_DELONGHI_ONE_SPACE   1558
#define IR_DELONGHI_ZERO_SPACE  510
#define IR_DELONGHI_GAP         100000
#define IR_DELONGHI_FREQ_HZ     38000

bool ir_delonghi_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
