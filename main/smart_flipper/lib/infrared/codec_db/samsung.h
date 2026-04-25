#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SAMSUNG_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SAMSUNG_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_SAMSUNG36_BITS              36
#define IR_SAMSUNG36_BITS_FIRST        16
#define IR_SAMSUNG36_HDR_MARK          4515
#define IR_SAMSUNG36_HDR_SPACE         4438
#define IR_SAMSUNG36_BIT_MARK          512
#define IR_SAMSUNG36_ONE_SPACE         1468
#define IR_SAMSUNG36_ZERO_SPACE        490
#define IR_SAMSUNG36_MIN_GAP           20000
#define IR_SAMSUNG36_FREQ_HZ           38000

#define IR_SAMSUNG_AC_STATE_BYTES      14
#define IR_SAMSUNG_AC_BITS             (IR_SAMSUNG_AC_STATE_BYTES * 8)
#define IR_SAMSUNG_AC_SECTION_LENGTH   7
#define IR_SAMSUNG_AC_HDR_MARK         690
#define IR_SAMSUNG_AC_HDR_SPACE        17844
#define IR_SAMSUNG_AC_SECTION_MARK     3086
#define IR_SAMSUNG_AC_SECTION_SPACE    8864
#define IR_SAMSUNG_AC_SECTION_GAP      2886
#define IR_SAMSUNG_AC_BIT_MARK         586
#define IR_SAMSUNG_AC_ONE_SPACE        1432
#define IR_SAMSUNG_AC_ZERO_SPACE       436
#define IR_SAMSUNG_AC_FREQ_HZ          38000

#define IR_SAMSUNG_AC256_STATE_BYTES   21
#define IR_SAMSUNG_AC256_BITS          (IR_SAMSUNG_AC256_STATE_BYTES * 8)

bool ir_samsung36_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

bool ir_samsung_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

bool ir_samsung_ac256_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
