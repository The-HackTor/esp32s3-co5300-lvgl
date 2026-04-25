#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MITSUBISHI_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_MITSUBISHI_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_MITSUBISHI_BITS         16
#define IR_MITSUBISHI_BIT_MARK     300
#define IR_MITSUBISHI_ONE_SPACE    2100
#define IR_MITSUBISHI_ZERO_SPACE   900
#define IR_MITSUBISHI_MIN_GAP      28080

#define IR_MITSUBISHI2_BITS        16
#define IR_MITSUBISHI2_HDR_MARK    8400
#define IR_MITSUBISHI2_HDR_SPACE   4200
#define IR_MITSUBISHI2_BIT_MARK    560
#define IR_MITSUBISHI2_ZERO_SPACE  520
#define IR_MITSUBISHI2_ONE_SPACE   1560
#define IR_MITSUBISHI2_MIN_GAP     28500

#define IR_MITSUBISHI_AC_STATE_BYTES 18
#define IR_MITSUBISHI_AC_BITS        (IR_MITSUBISHI_AC_STATE_BYTES * 8)
#define IR_MITSUBISHI_AC_HDR_MARK    3400
#define IR_MITSUBISHI_AC_HDR_SPACE   1750
#define IR_MITSUBISHI_AC_BIT_MARK    450
#define IR_MITSUBISHI_AC_ONE_SPACE   1300
#define IR_MITSUBISHI_AC_ZERO_SPACE  420
#define IR_MITSUBISHI_AC_RPT_MARK    440
#define IR_MITSUBISHI_AC_RPT_SPACE   15500

#define IR_MITSUBISHI112_STATE_BYTES 14
#define IR_MITSUBISHI112_BITS        (IR_MITSUBISHI112_STATE_BYTES * 8)
#define IR_MITSUBISHI112_HDR_MARK    3450
#define IR_MITSUBISHI112_HDR_SPACE   1696
#define IR_MITSUBISHI112_BIT_MARK    450
#define IR_MITSUBISHI112_ONE_SPACE   1250
#define IR_MITSUBISHI112_ZERO_SPACE  385
#define IR_MITSUBISHI112_GAP         100000

#define IR_MITSUBISHI136_STATE_BYTES 17
#define IR_MITSUBISHI136_BITS        (IR_MITSUBISHI136_STATE_BYTES * 8)
#define IR_MITSUBISHI136_HDR_MARK    3324
#define IR_MITSUBISHI136_HDR_SPACE   1474
#define IR_MITSUBISHI136_BIT_MARK    467
#define IR_MITSUBISHI136_ONE_SPACE   1137
#define IR_MITSUBISHI136_ZERO_SPACE  351
#define IR_MITSUBISHI136_GAP         100000

bool ir_mitsubishi_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);
bool ir_mitsubishi2_decode(const uint16_t *timings, size_t n_timings,
                           ir_decode_result_t *out);
bool ir_mitsubishi_ac_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_mitsubishi112_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_mitsubishi136_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
