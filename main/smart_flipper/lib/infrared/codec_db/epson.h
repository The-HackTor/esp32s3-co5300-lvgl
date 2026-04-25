#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_EPSON_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_EPSON_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_EPSON_BITS         32
#define IR_EPSON_HDR_MARK     9000
#define IR_EPSON_HDR_SPACE    4500
#define IR_EPSON_BIT_MARK     560
#define IR_EPSON_ONE_SPACE    1690
#define IR_EPSON_ZERO_SPACE   560
#define IR_EPSON_MIN_GAP      20000
#define IR_EPSON_FREQ_HZ      38000

bool ir_epson_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
