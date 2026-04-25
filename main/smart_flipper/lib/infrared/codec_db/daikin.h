#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DAIKIN_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_DAIKIN_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DAIKIN64_BITS         64
#define IR_DAIKIN64_HDR_MARK     4600
#define IR_DAIKIN64_HDR_SPACE    2500
#define IR_DAIKIN64_BIT_MARK     350
#define IR_DAIKIN64_ONE_SPACE    954
#define IR_DAIKIN64_ZERO_SPACE   382
#define IR_DAIKIN64_LDR_MARK     9800
#define IR_DAIKIN64_LDR_SPACE    9800
#define IR_DAIKIN64_GAP          20300

#define IR_DAIKIN128_BITS        128
#define IR_DAIKIN128_STATE_BYTES 16
#define IR_DAIKIN128_LDR_MARK    9800
#define IR_DAIKIN128_LDR_SPACE   9800
#define IR_DAIKIN128_HDR_MARK    4600
#define IR_DAIKIN128_HDR_SPACE   2500
#define IR_DAIKIN128_BIT_MARK    350
#define IR_DAIKIN128_ONE_SPACE   954
#define IR_DAIKIN128_ZERO_SPACE  382
#define IR_DAIKIN128_FOOTER_MARK 4600
#define IR_DAIKIN128_GAP         20300

#define IR_DAIKIN152_BITS        152
#define IR_DAIKIN152_STATE_BYTES 19
#define IR_DAIKIN152_LEADER_BITS 5
#define IR_DAIKIN152_HDR_MARK    3492
#define IR_DAIKIN152_HDR_SPACE   1718
#define IR_DAIKIN152_BIT_MARK    433
#define IR_DAIKIN152_ONE_SPACE   1529
#define IR_DAIKIN152_ZERO_SPACE  433
#define IR_DAIKIN152_GAP         25182

#define IR_DAIKIN160_BITS        160
#define IR_DAIKIN160_STATE_BYTES 20
#define IR_DAIKIN160_HDR_MARK    5000
#define IR_DAIKIN160_HDR_SPACE   2145
#define IR_DAIKIN160_BIT_MARK    342
#define IR_DAIKIN160_ONE_SPACE   1786
#define IR_DAIKIN160_ZERO_SPACE  700
#define IR_DAIKIN160_GAP         29650

#define IR_DAIKIN176_BITS        176
#define IR_DAIKIN176_STATE_BYTES 22
#define IR_DAIKIN176_HDR_MARK    5070
#define IR_DAIKIN176_HDR_SPACE   2140
#define IR_DAIKIN176_BIT_MARK    370
#define IR_DAIKIN176_ONE_SPACE   1780
#define IR_DAIKIN176_ZERO_SPACE  710
#define IR_DAIKIN176_GAP         29410

#define IR_DAIKIN200_BITS        200
#define IR_DAIKIN200_STATE_BYTES 25
#define IR_DAIKIN200_HDR_MARK    4920
#define IR_DAIKIN200_HDR_SPACE   2230
#define IR_DAIKIN200_BIT_MARK    290
#define IR_DAIKIN200_ONE_SPACE   1850
#define IR_DAIKIN200_ZERO_SPACE  780
#define IR_DAIKIN200_GAP         29400

#define IR_DAIKIN216_BITS        216
#define IR_DAIKIN216_STATE_BYTES 27
#define IR_DAIKIN216_HDR_MARK    3440
#define IR_DAIKIN216_HDR_SPACE   1750
#define IR_DAIKIN216_BIT_MARK    420
#define IR_DAIKIN216_ONE_SPACE   1300
#define IR_DAIKIN216_ZERO_SPACE  450
#define IR_DAIKIN216_GAP         29650

#define IR_DAIKIN_TOLERANCE      35

#define IR_DAIKIN_BITS                280
#define IR_DAIKIN_STATE_BYTES         35
#define IR_DAIKIN_HEADER_LENGTH       5
#define IR_DAIKIN_HDR_MARK            3650
#define IR_DAIKIN_HDR_SPACE           1623
#define IR_DAIKIN_BIT_MARK            428
#define IR_DAIKIN_ONE_SPACE           1280
#define IR_DAIKIN_ZERO_SPACE          428
#define IR_DAIKIN_GAP                 29000
#define IR_DAIKIN_SECTION1_LEN        8
#define IR_DAIKIN_SECTION2_LEN        8
#define IR_DAIKIN_SECTION3_LEN        (IR_DAIKIN_STATE_BYTES - \
                                       IR_DAIKIN_SECTION1_LEN - \
                                       IR_DAIKIN_SECTION2_LEN)

#define IR_DAIKIN2_BITS               312
#define IR_DAIKIN2_STATE_BYTES        39
#define IR_DAIKIN2_LEADER_MARK        10024
#define IR_DAIKIN2_LEADER_SPACE       25180
#define IR_DAIKIN2_HDR_MARK           3500
#define IR_DAIKIN2_HDR_SPACE          1728
#define IR_DAIKIN2_BIT_MARK           460
#define IR_DAIKIN2_ONE_SPACE          1270
#define IR_DAIKIN2_ZERO_SPACE         420
#define IR_DAIKIN2_GAP                (IR_DAIKIN2_LEADER_MARK + \
                                       IR_DAIKIN2_LEADER_SPACE)
#define IR_DAIKIN2_SECTION1_LEN       20
#define IR_DAIKIN2_SECTION2_LEN       (IR_DAIKIN2_STATE_BYTES - \
                                       IR_DAIKIN2_SECTION1_LEN)

#define IR_DAIKIN312_BITS             312
#define IR_DAIKIN312_STATE_BYTES      39
#define IR_DAIKIN312_HEADER_LENGTH    5
#define IR_DAIKIN312_HDR_MARK         3518
#define IR_DAIKIN312_HDR_SPACE        1688
#define IR_DAIKIN312_BIT_MARK         453
#define IR_DAIKIN312_ONE_SPACE        1275
#define IR_DAIKIN312_ZERO_SPACE       414
#define IR_DAIKIN312_HDR_GAP          25100
#define IR_DAIKIN312_SECTION_GAP      35512
#define IR_DAIKIN312_SECTION1_LEN     20
#define IR_DAIKIN312_SECTION2_LEN     (IR_DAIKIN312_STATE_BYTES - \
                                       IR_DAIKIN312_SECTION1_LEN)

bool ir_daikin64_decode(const uint16_t *timings, size_t n_timings,
                        ir_decode_result_t *out);
bool ir_daikin128_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin152_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin160_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin176_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin200_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin216_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);
bool ir_daikin_decode(const uint16_t *timings, size_t n_timings,
                      ir_decode_result_t *out);
bool ir_daikin2_decode(const uint16_t *timings, size_t n_timings,
                       ir_decode_result_t *out);
bool ir_daikin312_decode(const uint16_t *timings, size_t n_timings,
                         ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
