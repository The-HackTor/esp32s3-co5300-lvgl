#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ARGO_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_ARGO_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_ARGO_HDR_MARK       6400
#define IR_ARGO_HDR_SPACE      3300
#define IR_ARGO_BIT_MARK       400
#define IR_ARGO_ONE_SPACE      2200
#define IR_ARGO_ZERO_SPACE     900
#define IR_ARGO_GAP            100000
#define IR_ARGO_FREQ_HZ        38000

#define IR_ARGO_WREM2_BYTES    12
#define IR_ARGO_WREM2_BITS     (IR_ARGO_WREM2_BYTES * 8)

#define IR_ARGO_WREM3_AC_BYTES      6
#define IR_ARGO_WREM3_CFG_BYTES     4
#define IR_ARGO_WREM3_IFEEL_BYTES   2
#define IR_ARGO_WREM3_TIMER_BYTES   9

bool ir_argo_wrem2_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);
bool ir_argo_wrem3_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
