#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_EUROM_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_EUROM_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_EUROM_STATE_BYTES   12
#define IR_EUROM_BITS          (IR_EUROM_STATE_BYTES * 8)
#define IR_EUROM_HDR_MARK      3257
#define IR_EUROM_HDR_SPACE     3187
#define IR_EUROM_BIT_MARK      454
#define IR_EUROM_ONE_SPACE     1162
#define IR_EUROM_ZERO_SPACE    355
#define IR_EUROM_SPACE_GAP     50058
#define IR_EUROM_FREQ_HZ       38000

bool ir_eurom_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
