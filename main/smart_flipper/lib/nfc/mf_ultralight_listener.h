#ifndef MF_ULTRALIGHT_LISTENER_H
#define MF_ULTRALIGHT_LISTENER_H

#include <nfc_emulation.h>
#include <mf_ultralight.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MFU_MAX_PWD_ATTEMPTS 16

struct mfu_pwd_attempt {
    uint8_t pwd[4];
    uint8_t pack[2];
    bool    success;
};

struct mfu_ce_context {
    enum ce_state    state;
    struct mfu_dump *dump;
    atomic_bool      running;
    atomic_bool      stopped;
    bool             pwd_auth_ok;
    uint8_t          authlim_counter;
    bool             counter_bumped_this_activation;
    bool             in_comp_write;
    uint8_t          comp_write_page;
    TaskHandle_t     task;

    int64_t  first_rx_ms;
    int64_t  last_rx_ms;
    uint32_t read_cmd_count;
    uint32_t write_cmd_count;
    uint32_t auth_cmd_count;

    struct mfu_pwd_attempt attempts[MFU_MAX_PWD_ATTEMPTS];
    uint8_t attempt_count;
};

int  mfu_ce_start(struct mfu_ce_context *ctx, struct mfu_dump *dump);
int  mfu_ce_stop(struct mfu_ce_context *ctx);
bool mfu_ce_is_running(struct mfu_ce_context *ctx);

uint8_t mfu_ce_attempt_count(const struct mfu_ce_context *ctx);
const struct mfu_pwd_attempt *mfu_ce_attempts(const struct mfu_ce_context *ctx);
void mfu_ce_attempts_clear(struct mfu_ce_context *ctx);

uint32_t mfu_ce_read_count(const struct mfu_ce_context *ctx);
uint32_t mfu_ce_write_count(const struct mfu_ce_context *ctx);

#endif
