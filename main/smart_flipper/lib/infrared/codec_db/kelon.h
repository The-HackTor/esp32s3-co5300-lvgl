#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_KELON_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_KELON_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_KELON_BITS         48
#define IR_KELON_HDR_MARK     9000
#define IR_KELON_HDR_SPACE    4600
#define IR_KELON_BIT_MARK     560
#define IR_KELON_ONE_SPACE    1680
#define IR_KELON_ZERO_SPACE   600
#define IR_KELON_GAP          200000
#define IR_KELON_FREQ_HZ      38000

#define IR_KELON168_BITS              168
#define IR_KELON168_STATE_BYTES       21
#define IR_KELON168_SECTION1_BYTES    6
#define IR_KELON168_SECTION2_BYTES    8
#define IR_KELON168_SECTION3_BYTES    7
#define IR_KELON168_FOOTER_SPACE      8000

bool ir_kelon_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);
bool ir_kelon168_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
