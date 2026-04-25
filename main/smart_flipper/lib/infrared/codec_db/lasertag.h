#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LASERTAG_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LASERTAG_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_LASERTAG_BITS         13
#define IR_LASERTAG_HALF_PERIOD  333
#define IR_LASERTAG_FREQ_HZ      36000

bool ir_lasertag_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
