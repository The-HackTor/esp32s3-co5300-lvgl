#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_XMP_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_XMP_H_

#include "codec_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IR_XMP_BITS           64
#define IR_XMP_MARK           210
#define IR_XMP_BASE_SPACE     760
#define IR_XMP_SPACE_STEP     135
#define IR_XMP_FOOTER_SPACE   13000
#define IR_XMP_MESSAGE_GAP    80400
#define IR_XMP_WORD_SIZE      4
#define IR_XMP_SECTIONS       2
#define IR_XMP_MAX_WORD_VALUE ((1U << IR_XMP_WORD_SIZE) - 1U)
#define IR_XMP_REPEAT_CODE      0x8U
#define IR_XMP_REPEAT_CODE_ALT  0x9U
#define IR_XMP_FREQ_HZ        38000

bool ir_xmp_decode(const uint16_t *timings, size_t n_timings,
                   ir_decode_result_t *out);

#ifdef __cplusplus
}
#endif

#endif
