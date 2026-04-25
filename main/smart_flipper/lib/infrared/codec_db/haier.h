#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_HAIER_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_HAIER_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_HAIER_AC_HDR              3000
#define IR_HAIER_AC_HDR_GAP          4300
#define IR_HAIER_AC_BIT_MARK         520
#define IR_HAIER_AC_ONE_SPACE        1650
#define IR_HAIER_AC_ZERO_SPACE       650
#define IR_HAIER_AC_MIN_GAP          150000
#define IR_HAIER_AC_FREQ_HZ          38000

#define IR_HAIER_AC_STATE_BYTES      9
#define IR_HAIER_AC_BITS             (IR_HAIER_AC_STATE_BYTES * 8)
#define IR_HAIER_AC_YRW02_STATE_BYTES 14
#define IR_HAIER_AC_YRW02_BITS       (IR_HAIER_AC_YRW02_STATE_BYTES * 8)
#define IR_HAIER_AC160_STATE_BYTES   20
#define IR_HAIER_AC160_BITS          (IR_HAIER_AC160_STATE_BYTES * 8)
#define IR_HAIER_AC176_STATE_BYTES   22
#define IR_HAIER_AC176_BITS          (IR_HAIER_AC176_STATE_BYTES * 8)

bool ir_haier_ac_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);
bool ir_haier_ac_yrw02_decode(const uint16_t *timings, size_t n_timings,
                              ir_decode_result_t *out);
bool ir_haier_ac160_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);
bool ir_haier_ac176_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
