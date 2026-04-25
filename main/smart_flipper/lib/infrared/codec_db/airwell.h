#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIRWELL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AIRWELL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_AIRWELL_BITS              34
#define IR_AIRWELL_HALF_CLOCK_PERIOD 950
#define IR_AIRWELL_HDR_MARK          (3 * IR_AIRWELL_HALF_CLOCK_PERIOD)
#define IR_AIRWELL_HDR_SPACE         (3 * IR_AIRWELL_HALF_CLOCK_PERIOD)
#define IR_AIRWELL_FOOTER_MARK       (5 * IR_AIRWELL_HALF_CLOCK_PERIOD)
#define IR_AIRWELL_FREQ_HZ           38000

bool ir_airwell_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
