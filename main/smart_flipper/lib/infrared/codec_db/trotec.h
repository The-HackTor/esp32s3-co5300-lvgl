#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TROTEC_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_TROTEC_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_TROTEC_STATE_BYTES   9
#define IR_TROTEC_BITS          (IR_TROTEC_STATE_BYTES * 8)
#define IR_TROTEC_HDR_MARK      5952
#define IR_TROTEC_HDR_SPACE     7364
#define IR_TROTEC_BIT_MARK      592
#define IR_TROTEC_ONE_SPACE     1560
#define IR_TROTEC_ZERO_SPACE    592
#define IR_TROTEC_GAP           6184
#define IR_TROTEC_GAP_END       1500

#define IR_TROTEC3550_HDR_MARK  12000
#define IR_TROTEC3550_HDR_SPACE 5130
#define IR_TROTEC3550_BIT_MARK  550
#define IR_TROTEC3550_ONE_SPACE 1950
#define IR_TROTEC3550_ZERO_SPACE 500
#define IR_TROTEC3550_GAP       100000

bool ir_trotec_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);
bool ir_trotec3550_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
