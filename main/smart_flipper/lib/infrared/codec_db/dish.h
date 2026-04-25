#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DISH_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DISH_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DISH_BITS        16
#define IR_DISH_HDR_MARK    400
#define IR_DISH_HDR_SPACE   6100
#define IR_DISH_BIT_MARK    400
#define IR_DISH_ONE_SPACE   1700
#define IR_DISH_ZERO_SPACE  2800
#define IR_DISH_RPT_SPACE   6100
#define IR_DISH_FREQ_HZ     57600

bool ir_dish_decode(const uint16_t *timings, size_t n_timings,
                    ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
