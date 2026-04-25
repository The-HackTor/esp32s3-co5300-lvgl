#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BOSCH_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_BOSCH_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_BOSCH_HDR_MARK              4366
#define IR_BOSCH_HDR_SPACE             4415
#define IR_BOSCH_BIT_MARK              456
#define IR_BOSCH_ONE_SPACE             1645
#define IR_BOSCH_ZERO_SPACE            610
#define IR_BOSCH_FOOTER_SPACE          5235
#define IR_BOSCH_FREQ_HZ               38000

#define IR_BOSCH144_NR_OF_SECTIONS     3
#define IR_BOSCH144_BYTES_PER_SECTION  6
#define IR_BOSCH144_STATE_BYTES        (IR_BOSCH144_NR_OF_SECTIONS * \
                                        IR_BOSCH144_BYTES_PER_SECTION)
#define IR_BOSCH144_BITS               (IR_BOSCH144_STATE_BYTES * 8)

bool ir_bosch144_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
