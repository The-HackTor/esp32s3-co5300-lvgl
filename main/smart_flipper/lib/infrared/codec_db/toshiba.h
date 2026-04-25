#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TOSHIBA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TOSHIBA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TOSHIBA_AC_STATE_BYTES_SHORT 7
#define IR_TOSHIBA_AC_STATE_BYTES       9
#define IR_TOSHIBA_AC_STATE_BYTES_LONG  10
#define IR_TOSHIBA_AC_BITS              (IR_TOSHIBA_AC_STATE_BYTES * 8)
#define IR_TOSHIBA_AC_BITS_SHORT        (IR_TOSHIBA_AC_STATE_BYTES_SHORT * 8)
#define IR_TOSHIBA_AC_BITS_LONG         (IR_TOSHIBA_AC_STATE_BYTES_LONG * 8)
#define IR_TOSHIBA_AC_HDR_MARK          4400
#define IR_TOSHIBA_AC_HDR_SPACE         4300
#define IR_TOSHIBA_AC_BIT_MARK          580
#define IR_TOSHIBA_AC_ONE_SPACE         1600
#define IR_TOSHIBA_AC_ZERO_SPACE        490
#define IR_TOSHIBA_AC_MIN_GAP           4600

bool ir_toshiba_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
