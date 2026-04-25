#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CARRIER_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CARRIER_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_CARRIER_AC_BITS         32
#define IR_CARRIER_AC_HDR_MARK     8532
#define IR_CARRIER_AC_HDR_SPACE    4228
#define IR_CARRIER_AC_BIT_MARK     628
#define IR_CARRIER_AC_ONE_SPACE    1320
#define IR_CARRIER_AC_ZERO_SPACE   532
#define IR_CARRIER_AC_GAP          20000
#define IR_CARRIER_AC_FREQ_HZ      38000

#define IR_CARRIER_AC40_BITS       40
#define IR_CARRIER_AC40_HDR_MARK   8402
#define IR_CARRIER_AC40_HDR_SPACE  4166
#define IR_CARRIER_AC40_BIT_MARK   547
#define IR_CARRIER_AC40_ONE_SPACE  1540
#define IR_CARRIER_AC40_ZERO_SPACE 497
#define IR_CARRIER_AC40_GAP        150000

#define IR_CARRIER_AC64_BITS       64
#define IR_CARRIER_AC64_HDR_MARK   8940
#define IR_CARRIER_AC64_HDR_SPACE  4556
#define IR_CARRIER_AC64_BIT_MARK   503
#define IR_CARRIER_AC64_ONE_SPACE  1736
#define IR_CARRIER_AC64_ZERO_SPACE 615
#define IR_CARRIER_AC64_GAP        100000

#define IR_CARRIER_AC128_BITS      128
#define IR_CARRIER_AC128_HDR_MARK  4600
#define IR_CARRIER_AC128_HDR_SPACE 2600
#define IR_CARRIER_AC128_HDR2_MARK 9300
#define IR_CARRIER_AC128_HDR2_SPACE 5000
#define IR_CARRIER_AC128_BIT_MARK  340
#define IR_CARRIER_AC128_ONE_SPACE 1000
#define IR_CARRIER_AC128_ZERO_SPACE 400
#define IR_CARRIER_AC128_SECTION_GAP 20600
#define IR_CARRIER_AC128_INTER_SPACE 6700

#define IR_CARRIER_AC84_HDR_MARK        5850
#define IR_CARRIER_AC84_HDR_SPACE       1175
#define IR_CARRIER_AC84_ZERO            1175
#define IR_CARRIER_AC84_ONE             430
#define IR_CARRIER_AC84_GAP             100000
#define IR_CARRIER_AC84_BITS            84
#define IR_CARRIER_AC84_EXTRA_BITS      4
#define IR_CARRIER_AC84_STATE_LEN       11
#define IR_CARRIER_AC84_EXTRA_TOLERANCE 5

bool ir_carrier_ac_decode(const uint16_t *timings, size_t n_timings,
                          ir_decode_result_t *out);
bool ir_carrier_ac40_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);
bool ir_carrier_ac64_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);
bool ir_carrier_ac128_decode(const uint16_t *timings, size_t n_timings,
                             ir_decode_result_t *out);
bool ir_carrier_ac84_decode(const uint16_t *timings, size_t n_timings,
                            ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
