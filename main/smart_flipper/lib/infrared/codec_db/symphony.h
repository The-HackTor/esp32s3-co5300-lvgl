#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SYMPHONY_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_SYMPHONY_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_SYMPHONY_BITS         12
#define IR_SYMPHONY_ZERO_MARK    400
#define IR_SYMPHONY_ZERO_SPACE   1250
#define IR_SYMPHONY_ONE_MARK     1250
#define IR_SYMPHONY_ONE_SPACE    400
#define IR_SYMPHONY_FOOTER_GAP   (4 * (IR_SYMPHONY_ZERO_MARK + IR_SYMPHONY_ZERO_SPACE))
#define IR_SYMPHONY_FREQ_HZ      38000

bool ir_symphony_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
