#ifndef NFC_TRACE_H
#define NFC_TRACE_H

#include <stdint.h>
#include <stddef.h>

enum nfc_trace_dir_t {
    NFC_TRACE_DIR_R2C = 0,
    NFC_TRACE_DIR_C2R = 1,
};

static inline void nfc_trace_append(enum nfc_trace_dir_t dir,
                                    const uint8_t *data, size_t len)
{
    (void)dir; (void)data; (void)len;
}

#endif
