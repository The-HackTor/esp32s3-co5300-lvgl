#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CLIMA_BUTLER_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CLIMA_BUTLER_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_CLIMA_BUTLER_BITS        52
#define IR_CLIMA_BUTLER_BIT_MARK    511
#define IR_CLIMA_BUTLER_HDR_MARK    IR_CLIMA_BUTLER_BIT_MARK
#define IR_CLIMA_BUTLER_HDR_SPACE   3492
#define IR_CLIMA_BUTLER_ONE_SPACE   1540
#define IR_CLIMA_BUTLER_ZERO_SPACE  548
#define IR_CLIMA_BUTLER_GAP         100000
#define IR_CLIMA_BUTLER_FREQ_HZ     38000

bool ir_clima_butler_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
