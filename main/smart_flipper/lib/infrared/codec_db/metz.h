#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_METZ_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_METZ_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_METZ_BITS          19
#define IR_METZ_HDR_MARK      880
#define IR_METZ_HDR_SPACE     2336
#define IR_METZ_BIT_MARK      473
#define IR_METZ_ONE_SPACE     1640
#define IR_METZ_ZERO_SPACE    940
#define IR_METZ_MIN_GAP       100000
#define IR_METZ_FREQ_HZ       38000
#define IR_METZ_ADDRESS_BITS  3
#define IR_METZ_COMMAND_BITS  6

bool ir_metz_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
