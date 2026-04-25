#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ZEPEAL_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ZEPEAL_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ZEPEAL_BITS         16
#define IR_ZEPEAL_HDR_MARK     2330
#define IR_ZEPEAL_HDR_SPACE    3380
#define IR_ZEPEAL_ONE_MARK     1300
#define IR_ZEPEAL_ZERO_MARK    420
#define IR_ZEPEAL_ONE_SPACE    420
#define IR_ZEPEAL_ZERO_SPACE   1300
#define IR_ZEPEAL_FOOTER_MARK  420
#define IR_ZEPEAL_GAP          6750
#define IR_ZEPEAL_TOLERANCE    40
#define IR_ZEPEAL_SIGNATURE    0x6CU
#define IR_ZEPEAL_FREQ_HZ      38000

bool ir_zepeal_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
