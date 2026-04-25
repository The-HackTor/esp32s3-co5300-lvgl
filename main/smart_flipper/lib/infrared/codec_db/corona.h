#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CORONA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CORONA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_CORONA_AC_STATE_BYTES   21
#define IR_CORONA_AC_SECTION_BYTES 7
#define IR_CORONA_AC_SECTIONS      3
#define IR_CORONA_AC_BITS          (IR_CORONA_AC_STATE_BYTES * 8)
#define IR_CORONA_AC_BITS_SHORT    (IR_CORONA_AC_SECTION_BYTES * 8)
#define IR_CORONA_AC_HDR_MARK      3500
#define IR_CORONA_AC_HDR_SPACE     1680
#define IR_CORONA_AC_BIT_MARK      450
#define IR_CORONA_AC_ONE_SPACE     1270
#define IR_CORONA_AC_ZERO_SPACE    420
#define IR_CORONA_AC_SPACE_GAP     10800

bool ir_corona_ac_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
