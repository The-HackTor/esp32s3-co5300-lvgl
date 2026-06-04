#ifndef MF_CLASSIC_LISTENER_H
#define MF_CLASSIC_LISTENER_H

#include <nfc_emulation.h>
#include <nfc.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef void (*mfc_ce_nonce_cb_t)(const struct mfkey_nonce *nonce, void *ctx);

struct mfc_ce_context {
    enum ce_state        state;
    struct mfc_dump     *dump;
    struct crypto1_state crypto;
    uint8_t              auth_sector;
    uint8_t              write_block;
    int64_t              read_start_ms;
    bool                 crypto_active;
    bool                 pending_write;
    atomic_bool          running;
    atomic_bool          stopped;
    TaskHandle_t         task;

    mfc_ce_nonce_cb_t    nonce_cb;
    void                *nonce_cb_ctx;

    struct mfkey_nonce   nonces[MFKEY_MAX_NONCES];
    uint8_t              nonce_count;
};

int  mfc_ce_start(struct mfc_ce_context *ctx, struct mfc_dump *dump,
                  mfc_ce_nonce_cb_t cb, void *cb_ctx);
int  mfc_ce_stop(struct mfc_ce_context *ctx);
bool mfc_ce_is_running(struct mfc_ce_context *ctx);

uint8_t                    mfc_ce_nonce_count(const struct mfc_ce_context *ctx);
const struct mfkey_nonce  *mfc_ce_nonces(const struct mfc_ce_context *ctx);
void                       mfc_ce_nonces_clear(struct mfc_ce_context *ctx);

#endif
