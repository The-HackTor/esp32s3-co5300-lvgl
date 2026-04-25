#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_COOLIX_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_COOLIX_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_COOLIX_BITS         24
#define IR_COOLIX48_BITS       48
#define IR_COOLIX_HDR_MARK     4692
#define IR_COOLIX_HDR_SPACE    4416
#define IR_COOLIX_BIT_MARK     552
#define IR_COOLIX_ONE_SPACE    1656
#define IR_COOLIX_ZERO_SPACE   552
#define IR_COOLIX_MIN_GAP      5244
#define IR_COOLIX_FREQ_HZ      38000

bool ir_coolix_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

bool ir_coolix48_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
