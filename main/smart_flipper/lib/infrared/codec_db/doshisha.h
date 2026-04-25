#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DOSHISHA_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DOSHISHA_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DOSHISHA_BITS         40
#define IR_DOSHISHA_HDR_MARK     3412
#define IR_DOSHISHA_HDR_SPACE    1722
#define IR_DOSHISHA_BIT_MARK     420
#define IR_DOSHISHA_ONE_SPACE    1310
#define IR_DOSHISHA_ZERO_SPACE   452
#define IR_DOSHISHA_FREQ_HZ      38000

#define IR_DOSHISHA_SIGNATURE_MASK   0xFFFFFFFF00ULL
#define IR_DOSHISHA_SIGNATURE        0x800B304800ULL
#define IR_DOSHISHA_COMMAND_MASK     0xFEU
#define IR_DOSHISHA_CHANNEL_MASK     0x01U

bool ir_doshisha_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
