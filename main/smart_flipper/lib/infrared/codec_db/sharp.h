#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SHARP_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SHARP_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_SHARP_BITS           15
#define IR_SHARP_ADDRESS_BITS   5
#define IR_SHARP_COMMAND_BITS   8
#define IR_SHARP_BIT_MARK       260
#define IR_SHARP_ONE_SPACE      1820
#define IR_SHARP_ZERO_SPACE     780
#define IR_SHARP_GAP            43602
#define IR_SHARP_FREQ_HZ        38000

#define IR_SHARP_AC_BITS        104
#define IR_SHARP_AC_STATE_BYTES 13
#define IR_SHARP_AC_HDR_MARK    3800
#define IR_SHARP_AC_HDR_SPACE   1900
#define IR_SHARP_AC_BIT_MARK    470
#define IR_SHARP_AC_ONE_SPACE   1400
#define IR_SHARP_AC_ZERO_SPACE  500
#define IR_SHARP_AC_GAP         100000
#define IR_SHARP_AC_FREQ_HZ     38000

bool ir_sharp_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

bool ir_sharp_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
