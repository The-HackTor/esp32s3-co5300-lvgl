#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_KELVINATOR_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_KELVINATOR_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_KELVINATOR_HDR_MARK        9010
#define IR_KELVINATOR_HDR_SPACE       4505
#define IR_KELVINATOR_BIT_MARK        680
#define IR_KELVINATOR_ONE_SPACE       1530
#define IR_KELVINATOR_ZERO_SPACE      510
#define IR_KELVINATOR_GAP_SPACE       19975
#define IR_KELVINATOR_FREQ_HZ         38000

#define IR_KELVINATOR_STATE_BYTES     16
#define IR_KELVINATOR_BITS            (IR_KELVINATOR_STATE_BYTES * 8)
#define IR_KELVINATOR_CMD_FOOTER      0x2u
#define IR_KELVINATOR_CMD_FOOTER_BITS 3

bool ir_kelvinator_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
