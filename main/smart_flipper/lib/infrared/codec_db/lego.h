#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LEGO_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LEGO_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_LEGO_BITS                  16
#define IR_LEGO_BIT_MARK              158
#define IR_LEGO_HDR_SPACE             1026
#define IR_LEGO_ZERO_SPACE            263
#define IR_LEGO_ONE_SPACE             553
#define IR_LEGO_MIN_COMMAND_LENGTH    16000
#define IR_LEGO_FREQ_HZ               38000

bool ir_lego_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
