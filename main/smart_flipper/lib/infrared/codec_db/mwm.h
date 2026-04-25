#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MWM_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MWM_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MWM_TICK_US        417
#define IR_MWM_DELTA_US       150
#define IR_MWM_MIN_GAP        30000
#define IR_MWM_BITS_PER_FRAME 10
#define IR_MWM_FREQ_HZ        38000
#define IR_MWM_MAX_BYTES      IR_DECODE_STATE_MAX

bool ir_mwm_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
