#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BLUESTAR_HEAVY_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BLUESTAR_HEAVY_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_BLUESTAR_HEAVY_STATE_LENGTH 13
#define IR_BLUESTAR_HEAVY_BITS         (IR_BLUESTAR_HEAVY_STATE_LENGTH * 8)
#define IR_BLUESTAR_HEAVY_HDR_MARK     4912
#define IR_BLUESTAR_HEAVY_HDR_SPACE    5058
#define IR_BLUESTAR_HEAVY_BIT_MARK     465
#define IR_BLUESTAR_HEAVY_ONE_SPACE    572
#define IR_BLUESTAR_HEAVY_ZERO_SPACE   1548
#define IR_BLUESTAR_HEAVY_GAP          100000
#define IR_BLUESTAR_HEAVY_FREQ_HZ      38000

bool ir_bluestar_heavy_decode(const uint16_t *timings, size_t n_timings,
                              ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
