#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TRUMA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TRUMA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TRUMA_BITS         56
#define IR_TRUMA_LDR_MARK     20200
#define IR_TRUMA_LDR_SPACE    1000
#define IR_TRUMA_HDR_MARK     1800
#define IR_TRUMA_SPACE        630
#define IR_TRUMA_ONE_MARK     600
#define IR_TRUMA_ZERO_MARK    1200
#define IR_TRUMA_FOOTER_MARK  IR_TRUMA_ONE_MARK
#define IR_TRUMA_MIN_GAP      100000
#define IR_TRUMA_FREQ_HZ      38000

bool ir_truma_decode(const uint16_t *timings, size_t n_timings,
                     ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
