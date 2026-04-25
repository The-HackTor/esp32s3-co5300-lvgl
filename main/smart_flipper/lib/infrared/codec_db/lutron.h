#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LUTRON_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_LUTRON_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_LUTRON_BITS        35
#define IR_LUTRON_TICK_US     2288
#define IR_LUTRON_GAP         150000
#define IR_LUTRON_FREQ_HZ     40000

bool ir_lutron_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
