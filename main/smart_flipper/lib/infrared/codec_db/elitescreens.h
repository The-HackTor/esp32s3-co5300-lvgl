#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ELITESCREENS_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ELITESCREENS_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ELITESCREENS_BITS       32
#define IR_ELITESCREENS_ONE_PULSE  470
#define IR_ELITESCREENS_ZERO_PULSE 1214
#define IR_ELITESCREENS_GAP        29200
#define IR_ELITESCREENS_FREQ_HZ    38000

bool ir_elitescreens_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
