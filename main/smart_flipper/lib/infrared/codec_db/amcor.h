#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AMCOR_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_AMCOR_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_AMCOR_STATE_LENGTH 8
#define IR_AMCOR_BITS         (IR_AMCOR_STATE_LENGTH * 8)
#define IR_AMCOR_HDR_MARK     8200
#define IR_AMCOR_HDR_SPACE    4200
#define IR_AMCOR_ONE_MARK     1500
#define IR_AMCOR_ZERO_MARK    600
#define IR_AMCOR_ONE_SPACE    IR_AMCOR_ZERO_MARK
#define IR_AMCOR_ZERO_SPACE   IR_AMCOR_ONE_MARK
#define IR_AMCOR_FOOTER_MARK  1900
#define IR_AMCOR_GAP          34300
#define IR_AMCOR_TOLERANCE    40
#define IR_AMCOR_FREQ_HZ      38000

bool ir_amcor_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
