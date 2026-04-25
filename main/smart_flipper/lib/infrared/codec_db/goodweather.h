#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GOODWEATHER_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GOODWEATHER_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_GOODWEATHER_BITS            48
#define IR_GOODWEATHER_HDR_MARK        6820
#define IR_GOODWEATHER_HDR_SPACE       6820
#define IR_GOODWEATHER_BIT_MARK        580
#define IR_GOODWEATHER_ONE_SPACE       1860
#define IR_GOODWEATHER_ZERO_SPACE      580
#define IR_GOODWEATHER_EXTRA_TOLERANCE 12
#define IR_GOODWEATHER_FREQ_HZ         38000

bool ir_goodweather_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
