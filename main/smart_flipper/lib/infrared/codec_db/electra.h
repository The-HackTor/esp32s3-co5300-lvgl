#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ELECTRA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ELECTRA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ELECTRA_AC_STATE_LENGTH 13
#define IR_ELECTRA_AC_BITS         (IR_ELECTRA_AC_STATE_LENGTH * 8)
#define IR_ELECTRA_AC_HDR_MARK     9166
#define IR_ELECTRA_AC_HDR_SPACE    4470
#define IR_ELECTRA_AC_BIT_MARK     646
#define IR_ELECTRA_AC_ONE_SPACE    1647
#define IR_ELECTRA_AC_ZERO_SPACE   547
#define IR_ELECTRA_AC_GAP          100000
#define IR_ELECTRA_AC_FREQ_HZ      38000

bool ir_electra_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
