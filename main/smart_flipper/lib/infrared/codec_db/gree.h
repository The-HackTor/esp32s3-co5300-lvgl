#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GREE_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_GREE_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_GREE_HDR_MARK         9000
#define IR_GREE_HDR_SPACE        4500
#define IR_GREE_BIT_MARK         620
#define IR_GREE_ONE_SPACE        1600
#define IR_GREE_ZERO_SPACE       540
#define IR_GREE_MSG_SPACE        19980
#define IR_GREE_FREQ_HZ          38000

#define IR_GREE_STATE_BYTES      8
#define IR_GREE_BITS             (IR_GREE_STATE_BYTES * 8)
#define IR_GREE_BLOCK_FOOTER     0x2u
#define IR_GREE_BLOCK_FOOTER_BITS 3

bool ir_gree_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
