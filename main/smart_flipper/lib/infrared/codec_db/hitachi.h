#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_HITACHI_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_HITACHI_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_HITACHI_FREQ_HZ            38000
#define IR_HITACHI_AC_MIN_GAP         100000

#define IR_HITACHI_AC_HDR_MARK        3300
#define IR_HITACHI_AC_HDR_SPACE       1700
#define IR_HITACHI_AC_BIT_MARK        400
#define IR_HITACHI_AC_ONE_SPACE       1250
#define IR_HITACHI_AC_ZERO_SPACE      500

#define IR_HITACHI_AC1_HDR_MARK       3400
#define IR_HITACHI_AC1_HDR_SPACE      3400
#define IR_HITACHI_AC1_STATE_BYTES    13
#define IR_HITACHI_AC1_BITS           (IR_HITACHI_AC1_STATE_BYTES * 8)

#define IR_HITACHI_AC3_HDR_MARK       3400
#define IR_HITACHI_AC3_HDR_SPACE      1660
#define IR_HITACHI_AC3_BIT_MARK       460
#define IR_HITACHI_AC3_ONE_SPACE      1250
#define IR_HITACHI_AC3_ZERO_SPACE     410
#define IR_HITACHI_AC3_MAX_BYTES      27
#define IR_HITACHI_AC3_MIN_BYTES      15

#define IR_HITACHI_AC_STATE_BYTES     28
#define IR_HITACHI_AC_BITS            (IR_HITACHI_AC_STATE_BYTES * 8)

#define IR_HITACHI_AC2_STATE_BYTES    53
#define IR_HITACHI_AC2_BITS           (IR_HITACHI_AC2_STATE_BYTES * 8)

#define IR_HITACHI_AC264_STATE_BYTES  33
#define IR_HITACHI_AC264_BITS         (IR_HITACHI_AC264_STATE_BYTES * 8)

#define IR_HITACHI_AC296_STATE_BYTES  37
#define IR_HITACHI_AC296_BITS         (IR_HITACHI_AC296_STATE_BYTES * 8)

#define IR_HITACHI_AC344_STATE_BYTES  43
#define IR_HITACHI_AC344_BITS         (IR_HITACHI_AC344_STATE_BYTES * 8)

#define IR_HITACHI_AC424_STATE_BYTES  53
#define IR_HITACHI_AC424_BITS         (IR_HITACHI_AC424_STATE_BYTES * 8)
#define IR_HITACHI_AC424_LDR_MARK     29784
#define IR_HITACHI_AC424_LDR_SPACE    49290
#define IR_HITACHI_AC424_HDR_MARK     3416
#define IR_HITACHI_AC424_HDR_SPACE    1604
#define IR_HITACHI_AC424_BIT_MARK     463
#define IR_HITACHI_AC424_ONE_SPACE    1208
#define IR_HITACHI_AC424_ZERO_SPACE   372

bool ir_hitachi_ac1_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);
bool ir_hitachi_ac3_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);
bool ir_hitachi_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);
bool ir_hitachi_ac2_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);
bool ir_hitachi_ac264_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_hitachi_ac296_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_hitachi_ac344_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_hitachi_ac424_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
