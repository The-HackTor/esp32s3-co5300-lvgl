#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TCL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TCL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TCL112_HDR_MARK         3000
#define IR_TCL112_HDR_SPACE        1650
#define IR_TCL112_BIT_MARK         500
#define IR_TCL112_ONE_SPACE        1050
#define IR_TCL112_ZERO_SPACE       325
#define IR_TCL112_GAP              100000
#define IR_TCL112_FREQ_HZ          38000
#define IR_TCL112_TOLERANCE_EXTRA  5

#define IR_TCL112_STATE_BYTES      14
#define IR_TCL112_BITS             (IR_TCL112_STATE_BYTES * 8)

#define IR_TCL96_AC_HDR_MARK       1056
#define IR_TCL96_AC_HDR_SPACE      550
#define IR_TCL96_AC_BIT_MARK       600
#define IR_TCL96_AC_GAP            100000
#define IR_TCL96_AC_ZERO_ZERO_SPACE 360
#define IR_TCL96_AC_ZERO_ONE_SPACE  838
#define IR_TCL96_AC_ONE_ZERO_SPACE  2182
#define IR_TCL96_AC_ONE_ONE_SPACE   1444
#define IR_TCL96_AC_STATE_BYTES    12
#define IR_TCL96_AC_BITS           (IR_TCL96_AC_STATE_BYTES * 8)

bool ir_tcl112_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

bool ir_tcl96_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
