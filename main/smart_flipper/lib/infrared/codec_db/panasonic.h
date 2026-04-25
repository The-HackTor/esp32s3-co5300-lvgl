#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_PANASONIC_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_PANASONIC_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_PANASONIC_BITS              48
#define IR_PANASONIC_HDR_MARK          3456
#define IR_PANASONIC_HDR_SPACE         1728
#define IR_PANASONIC_BIT_MARK          432
#define IR_PANASONIC_ONE_SPACE         1296
#define IR_PANASONIC_ZERO_SPACE        432
#define IR_PANASONIC_END_GAP           5000
#define IR_PANASONIC_MIN_GAP           74736
#define IR_PANASONIC_FREQ_HZ           36700

#define IR_PANASONIC_AC_BITS           216
#define IR_PANASONIC_AC_STATE_BYTES    27
#define IR_PANASONIC_AC_SECTION1_LEN   8
#define IR_PANASONIC_AC_SECTION_GAP    10000

bool ir_panasonic_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_panasonic_ac_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);
bool ir_panasonic_ac32_decode(const uint16_t *timings, size_t n_timings,
                              ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
