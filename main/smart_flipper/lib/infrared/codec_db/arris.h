#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ARRIS_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ARRIS_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ARRIS_BITS              32
#define IR_ARRIS_HALF_CLOCK_PERIOD 320
#define IR_ARRIS_HDR_MARK          (8 * IR_ARRIS_HALF_CLOCK_PERIOD)
#define IR_ARRIS_HDR_SPACE         (6 * IR_ARRIS_HALF_CLOCK_PERIOD)
#define IR_ARRIS_CHECKSUM_SIZE     4
#define IR_ARRIS_COMMAND_SIZE      19
#define IR_ARRIS_RELEASE_BIT       (IR_ARRIS_CHECKSUM_SIZE + IR_ARRIS_COMMAND_SIZE)
#define IR_ARRIS_FREQ_HZ           38000

bool ir_arris_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
