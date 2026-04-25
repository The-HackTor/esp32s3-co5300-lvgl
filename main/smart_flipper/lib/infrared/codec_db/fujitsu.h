#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_FUJITSU_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_FUJITSU_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_FUJITSU_AC_HDR_MARK         3324
#define IR_FUJITSU_AC_HDR_SPACE        1574
#define IR_FUJITSU_AC_BIT_MARK         448
#define IR_FUJITSU_AC_ONE_SPACE        1182
#define IR_FUJITSU_AC_ZERO_SPACE       390
#define IR_FUJITSU_AC_MIN_GAP          8100
#define IR_FUJITSU_AC_FREQ_HZ          38000

#define IR_FUJITSU_AC_STATE_MAX        16
#define IR_FUJITSU_AC_STATE_MIN        7
#define IR_FUJITSU_AC_PREFIX_BYTES     5

#define IR_FUJITSU_AC_EXTRA_TOLERANCE  5

bool ir_fujitsu_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
