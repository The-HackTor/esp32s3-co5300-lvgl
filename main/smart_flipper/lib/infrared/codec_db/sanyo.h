#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SANYO_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SANYO_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_SANYO_LC7461_ADDRESS_BITS  13
#define IR_SANYO_LC7461_COMMAND_BITS  8
#define IR_SANYO_LC7461_BITS          ((IR_SANYO_LC7461_ADDRESS_BITS + IR_SANYO_LC7461_COMMAND_BITS) * 2)
#define IR_SANYO_LC7461_HDR_MARK      9000
#define IR_SANYO_LC7461_HDR_SPACE     4500
#define IR_SANYO_LC7461_BIT_MARK      560
#define IR_SANYO_LC7461_ONE_SPACE     1690
#define IR_SANYO_LC7461_ZERO_SPACE    560
#define IR_SANYO_LC7461_MIN_GAP       20000
#define IR_SANYO_LC7461_FREQ_HZ       38000

#define IR_SANYO_AC_STATE_BYTES       9
#define IR_SANYO_AC_BITS              (IR_SANYO_AC_STATE_BYTES * 8)
#define IR_SANYO_AC_HDR_MARK          8500
#define IR_SANYO_AC_HDR_SPACE         4200
#define IR_SANYO_AC_BIT_MARK          500
#define IR_SANYO_AC_ONE_SPACE         1600
#define IR_SANYO_AC_ZERO_SPACE        550
#define IR_SANYO_AC_GAP               100000
#define IR_SANYO_AC_FREQ_HZ           38000

#define IR_SANYO_AC88_STATE_BYTES     11
#define IR_SANYO_AC88_BITS            (IR_SANYO_AC88_STATE_BYTES * 8)
#define IR_SANYO_AC88_HDR_MARK        5400
#define IR_SANYO_AC88_HDR_SPACE       2000
#define IR_SANYO_AC88_BIT_MARK        500
#define IR_SANYO_AC88_ONE_SPACE       1500
#define IR_SANYO_AC88_ZERO_SPACE      750
#define IR_SANYO_AC88_GAP             3675
#define IR_SANYO_AC88_FREQ_HZ         38000
#define IR_SANYO_AC88_TOLERANCE       30

#define IR_SANYO_AC152_STATE_BYTES    19
#define IR_SANYO_AC152_BITS           (IR_SANYO_AC152_STATE_BYTES * 8)
#define IR_SANYO_AC152_HDR_MARK       3300
#define IR_SANYO_AC152_HDR_SPACE      1725
#define IR_SANYO_AC152_BIT_MARK       440
#define IR_SANYO_AC152_ONE_SPACE      1290
#define IR_SANYO_AC152_ZERO_SPACE     405
#define IR_SANYO_AC152_GAP            100000
#define IR_SANYO_AC152_FREQ_HZ        38000
#define IR_SANYO_AC152_TOLERANCE      38

bool ir_sanyo_lc7461_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);

bool ir_sanyo_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

bool ir_sanyo_ac88_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

bool ir_sanyo_ac152_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
