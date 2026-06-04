#ifndef MFC_POLLER_H
#define MFC_POLLER_H

#include <nfc.h>
#include <stdbool.h>

enum mfc_poller_phase {
    MFC_PHASE_DETECT,
    MFC_PHASE_BACKDOOR,
    MFC_PHASE_KEY_REUSE,
    MFC_PHASE_DICT_ATTACK,
    MFC_PHASE_NESTED,
    MFC_PHASE_READ,
    MFC_PHASE_DONE,
};

typedef void (*mfc_poller_progress_cb)(enum mfc_poller_phase phase,
                                       uint8_t sectors_read,
                                       uint8_t sectors_total,
                                       void *ctx);

int mfc_poller_dump(struct mfc_dump *dump,
                    mfc_poller_progress_cb progress, void *ctx);

extern const uint8_t mfc_backdoor_keys[][6];

#endif
