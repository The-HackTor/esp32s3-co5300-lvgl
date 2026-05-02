#ifndef HW_TYPES_H
#define HW_TYPES_H

#include <nfc.h>
#include <nfc_emulation.h>
#include <mf_ultralight.h>
#include <stdint.h>
#include <stdbool.h>

enum nfc_card_type {
	NFC_CARD_NONE,
	NFC_CARD_MFC,
	NFC_CARD_MFU,
};

#define SUBGHZ_RAW_MAX_SAMPLES    1024
#define SUBGHZ_RAW_FILE_PATH_LEN  80
#define SUBGHZ_PROTOCOL_NAME_MAX  16

#define SIM_SUBGHZ_MAX_SAMPLES SUBGHZ_RAW_MAX_SAMPLES

struct subghz_raw {
    int32_t  samples[SUBGHZ_RAW_MAX_SAMPLES];     /* preview (first N) */
    uint16_t count;                                /* samples in preview */
    uint32_t full_count;                           /* total streamed to file */
    char     file_path[SUBGHZ_RAW_FILE_PATH_LEN];  /* "" = not file-backed */
};

typedef struct subghz_raw SimSubghzRaw;

typedef struct {
    const char *protocol;
    uint64_t    data;
    uint8_t     bits;
    uint32_t    te;
} SimSubghzDecoded;

typedef struct {
    uint32_t freq_khz;
    float    rssi;
} SimSubghzFreqResult;

#define SUBGHZ_HISTORY_MAX 50

typedef struct {
    char     protocol[SUBGHZ_PROTOCOL_NAME_MAX];
    uint64_t data;
    uint8_t  bits;
    uint32_t te;
    uint32_t freq_khz;
    int64_t  timestamp_ms;
    uint8_t  repeat_count;
    uint32_t serial;
    uint16_t counter;
    uint8_t  button;
    uint8_t  proto_type;
} SubghzHistoryEntry;

#endif
