#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_VOLTAS_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_VOLTAS_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_VOLTAS_STATE_BYTES 10
#define IR_VOLTAS_BITS        80
#define IR_VOLTAS_BIT_MARK    1026
#define IR_VOLTAS_ONE_SPACE   2553
#define IR_VOLTAS_ZERO_SPACE  554
#define IR_VOLTAS_GAP         100000

bool ir_voltas_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
