#include "mf_ultralight_listener.h"
#include "ce_chip.h"
#include "nfc_poller.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>
#include <string.h>
#include <nfc.h>

static const char *TAG = "mfu_ce";

#define MFU_CE_STACK_SIZE    4096
#define MFU_CE_PRIORITY      8
#define MFU_CE_RX_TIMEOUT_MS 20

#define MFU_ACK              0x0A
#define MFU_NACK             0x00
#define MFU_AUTH_NAK         0x04

#define MFU_CMD_READ         0x30
#define MFU_CMD_FAST_READ    0x3A
#define MFU_CMD_WRITE        0xA2
#define MFU_CMD_COMP_WRITE   0xA0
#define MFU_CMD_GET_VERSION  0x60
#define MFU_CMD_READ_SIG     0x3C
#define MFU_CMD_READ_CNT     0x39
#define MFU_CMD_CHECK_TEAR   0x3E
#define MFU_CMD_PWD_AUTH     0x1B
#define MFU_CMD_HALT         0x50

#define MFU_TEARING_OK       0xBD

#define CFG1_PROT_MASK           0x80
#define CFG1_CFGLCK_MASK         0x40
#define CFG1_NFC_CNT_EN_MASK     0x10
#define CFG1_NFC_CNT_PWD_MASK    0x08
#define CFG1_AUTHLIM_MASK        0x07

enum mfu_cmd_result {
    MFU_CMD_CONTINUE = 0,
    MFU_CMD_SLEEP,
    MFU_CMD_IDLE,
};

struct mfu_layout {
    uint16_t total_pages;
    uint16_t dyn_lock_page;
    uint16_t cfg0_page;
    uint16_t cfg1_page;
    uint16_t pwd_page;
    uint16_t pack_page;
    bool     has_auth;
    bool     has_counter;
};

static const uint8_t ntag216_default_version[8] = {
    0x00, 0x04, 0x04, 0x02, 0x01, 0x00, 0x13, 0x03
};

static const uint8_t default_pack[2] = { 0x00, 0x00 };
static const uint8_t default_pwd[4]  = { 0xFF, 0xFF, 0xFF, 0xFF };

static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

static void mfu_layout_get(enum mfu_type type, struct mfu_layout *out)
{
    memset(out, 0, sizeof(*out));
    out->total_pages = mfu_type_pages(type);

    switch(type) {
    case MFU_TYPE_NTAG213:
        out->dyn_lock_page = 0x28; out->cfg0_page = 0x29; out->cfg1_page = 0x2A;
        out->pwd_page = 0x2B;      out->pack_page = 0x2C;
        out->has_auth = true;      out->has_counter = true;
        break;
    case MFU_TYPE_NTAG215:
        out->dyn_lock_page = 0x82; out->cfg0_page = 0x83; out->cfg1_page = 0x84;
        out->pwd_page = 0x85;      out->pack_page = 0x86;
        out->has_auth = true;      out->has_counter = true;
        break;
    case MFU_TYPE_NTAG216:
        out->dyn_lock_page = 0xE2; out->cfg0_page = 0xE3; out->cfg1_page = 0xE4;
        out->pwd_page = 0xE5;      out->pack_page = 0xE6;
        out->has_auth = true;      out->has_counter = true;
        break;
    case MFU_TYPE_UL_EV1_21:
        out->dyn_lock_page = 0x24; out->cfg0_page = 0x25; out->cfg1_page = 0x26;
        out->pwd_page = 0x27;      out->pack_page = 0x28;
        out->has_auth = true;      out->has_counter = true;
        break;
    case MFU_TYPE_UL_EV1_11:
        out->dyn_lock_page = 0x10; out->cfg0_page = 0x11; out->cfg1_page = 0x12;
        out->pwd_page = 0x12;      out->pack_page = 0x13;
        out->has_auth = true;      out->has_counter = true;
        break;
    default:
        break;
    }
}

static uint8_t mfu_cfg0_auth0(const struct mfu_ce_context *ctx,
                              const struct mfu_layout *L)
{
    if(!L->has_auth || L->cfg0_page == 0) return 0xFF;
    if(L->cfg0_page >= MFU_MAX_PAGES) return 0xFF;
    if(!ctx->dump->page_read[L->cfg0_page]) return 0xFF;
    return ctx->dump->pages[L->cfg0_page][3];
}

static uint8_t mfu_cfg1_access(const struct mfu_ce_context *ctx,
                               const struct mfu_layout *L)
{
    if(!L->has_auth || L->cfg1_page == 0) return 0x00;
    if(L->cfg1_page >= MFU_MAX_PAGES) return 0x00;
    if(!ctx->dump->page_read[L->cfg1_page]) return 0x00;
    return ctx->dump->pages[L->cfg1_page][0];
}

static bool mfu_page_protected_read(const struct mfu_ce_context *ctx,
                                    const struct mfu_layout *L, uint16_t page)
{
    if(!L->has_auth) return false;
    uint8_t auth0 = mfu_cfg0_auth0(ctx, L);
    uint8_t access = mfu_cfg1_access(ctx, L);
    bool prot_both = (access & CFG1_PROT_MASK) != 0;
    if(!prot_both) return false;
    if(page < auth0) return false;
    return !ctx->pwd_auth_ok;
}

static bool mfu_page_protected_write(const struct mfu_ce_context *ctx,
                                     const struct mfu_layout *L, uint16_t page)
{
    if(!L->has_auth) return false;
    uint8_t auth0 = mfu_cfg0_auth0(ctx, L);
    if(page < auth0) return false;
    return !ctx->pwd_auth_ok;
}

static bool mfu_static_lock_page(const struct mfu_ce_context *ctx, uint16_t page)
{
    if(page < 3 || page > 0x0F) return false;
    if(!ctx->dump->page_read[2]) return false;
    uint16_t lock_bits = ((uint16_t)ctx->dump->pages[2][3] << 8) |
                         ctx->dump->pages[2][2];
    if(page == 3) return (lock_bits & (1U << 3)) != 0;
    return (lock_bits & (1U << page)) != 0;
}

static bool mfu_dynamic_lock_page(const struct mfu_ce_context *ctx,
                                  const struct mfu_layout *L, uint16_t page)
{
    if(L->dyn_lock_page == 0 || L->dyn_lock_page >= MFU_MAX_PAGES) return false;
    if(page < 0x10 || page >= L->dyn_lock_page) return false;
    if(!ctx->dump->page_read[L->dyn_lock_page]) return false;

    uint32_t locks = ((uint32_t)ctx->dump->pages[L->dyn_lock_page][2] << 16) |
                     ((uint32_t)ctx->dump->pages[L->dyn_lock_page][1] << 8) |
                     (uint32_t)ctx->dump->pages[L->dyn_lock_page][0];

    uint16_t user_end = L->dyn_lock_page;
    uint16_t user_span = user_end - 0x10;
    if(user_span == 0) return false;

    uint16_t rel = page - 0x10;
    uint16_t lock_idx = (rel * 16) / user_span;
    if(lock_idx > 15) lock_idx = 15;

    return (locks & (1U << lock_idx)) != 0;
}

static void mfu_read_page_masked(const struct mfu_ce_context *ctx,
                                 const struct mfu_layout *L, uint16_t page,
                                 uint8_t out[4])
{
    if(L->has_auth && page == L->pwd_page) { memset(out, 0, 4); return; }
    if(L->has_auth && page == L->pack_page) {
        if(ctx->dump->page_read[page]) {
            out[0] = ctx->dump->pages[page][0];
            out[1] = ctx->dump->pages[page][1];
        } else {
            out[0] = default_pack[0]; out[1] = default_pack[1];
        }
        out[2] = 0; out[3] = 0;
        return;
    }
    if(page < MFU_MAX_PAGES && ctx->dump->page_read[page])
        memcpy(out, ctx->dump->pages[page], 4);
    else
        memset(out, 0, 4);
}

static bool mfu_is_writable(const struct mfu_ce_context *ctx,
                            const struct mfu_layout *L, uint16_t page)
{
    if(page < 2) return false;
    if(page >= L->total_pages) return false;
    if(mfu_static_lock_page(ctx, page)) return false;
    if(mfu_dynamic_lock_page(ctx, L, page)) return false;
    if(mfu_page_protected_write(ctx, L, page)) return false;

    if(L->has_auth && L->cfg1_page != 0 && page == L->cfg1_page) {
        uint8_t access = mfu_cfg1_access(ctx, L);
        if(access & CFG1_CFGLCK_MASK) return false;
    }
    if(L->has_auth && L->cfg0_page != 0 && page == L->cfg0_page) {
        uint8_t access = mfu_cfg1_access(ctx, L);
        if(access & CFG1_CFGLCK_MASK) return false;
    }
    return true;
}

static void mfu_apply_write(struct mfu_ce_context *ctx,
                            const struct mfu_layout *L, uint16_t page,
                            const uint8_t data[4])
{
    if(page >= MFU_MAX_PAGES) return;

    if(page == 2) {
        uint8_t cur[4] = {0};
        if(ctx->dump->page_read[2]) memcpy(cur, ctx->dump->pages[2], 4);
        cur[2] |= data[2]; cur[3] |= data[3];
        memcpy(ctx->dump->pages[2], cur, 4);
    } else if(page == 3) {
        uint8_t cur[4] = {0};
        if(ctx->dump->page_read[3]) memcpy(cur, ctx->dump->pages[3], 4);
        for(int i = 0; i < 4; i++) cur[i] |= data[i];
        memcpy(ctx->dump->pages[3], cur, 4);
    } else if(L->has_auth && L->dyn_lock_page != 0 && page == L->dyn_lock_page) {
        uint8_t cur[4] = {0};
        if(ctx->dump->page_read[page]) memcpy(cur, ctx->dump->pages[page], 4);
        for(int i = 0; i < 3; i++) cur[i] |= data[i];
        cur[3] = data[3];
        memcpy(ctx->dump->pages[page], cur, 4);
    } else {
        memcpy(ctx->dump->pages[page], data, 4);
    }
    ctx->dump->page_read[page] = true;
    ctx->write_cmd_count++;
}

static void mfu_maybe_bump_counter(struct mfu_ce_context *ctx,
                                   const struct mfu_layout *L)
{
    if(!L->has_counter) return;
    if(ctx->counter_bumped_this_activation) return;

    uint8_t access = mfu_cfg1_access(ctx, L);
    if((access & CFG1_NFC_CNT_EN_MASK) == 0) {
        ctx->counter_bumped_this_activation = true;
        return;
    }

    ctx->dump->nfc_counter++;
    if(ctx->dump->nfc_counter > 0xFFFFFF) ctx->dump->nfc_counter = 0xFFFFFF;
    ctx->counter_bumped_this_activation = true;
}

static void mfu_record_attempt(struct mfu_ce_context *ctx,
                               const uint8_t pwd[4], bool success,
                               const uint8_t pack[2])
{
    if(ctx->attempt_count >= MFU_MAX_PWD_ATTEMPTS) return;
    for(uint8_t i = 0; i < ctx->attempt_count; i++) {
        if(memcmp(ctx->attempts[i].pwd, pwd, 4) == 0) return;
    }
    struct mfu_pwd_attempt *a = &ctx->attempts[ctx->attempt_count++];
    memcpy(a->pwd, pwd, 4);
    memcpy(a->pack, pack, 2);
    a->success = success;
    ESP_LOGI(TAG, "pwd %02X%02X%02X%02X %s",
             pwd[0], pwd[1], pwd[2], pwd[3], success ? "OK" : "fail");
}

static void mfu_send_short(uint8_t nibble)
{
    uint8_t byte = nibble & 0x0F;
    ce_chip_fifo_tx(&byte, 4);
}

static void mfu_send_nak(void)      { mfu_send_short(MFU_NACK); ce_chip_prepare_rx(); }
static void mfu_send_auth_nak(void) { mfu_send_short(MFU_AUTH_NAK); ce_chip_prepare_rx(); }
static void mfu_send_ack(void)      { mfu_send_short(MFU_ACK); ce_chip_prepare_rx(); }
static void mfu_send_std(const uint8_t *data, uint16_t bytes)
{
    ce_chip_fifo_tx_with_crc(data, bytes);
    ce_chip_prepare_rx();
}

static void mfu_handle_read(struct mfu_ce_context *ctx,
                            const struct mfu_layout *L, const uint8_t *cmd, size_t len)
{
    if(len < 2) { mfu_send_nak(); return; }
    uint8_t start = cmd[1];
    if(L->total_pages == 0 || start >= L->total_pages) { mfu_send_nak(); return; }
    for(int i = 0; i < 4; i++) {
        uint16_t p = (start + i) % L->total_pages;
        if(mfu_page_protected_read(ctx, L, p)) { mfu_send_nak(); return; }
    }
    ctx->read_cmd_count++;
    mfu_maybe_bump_counter(ctx, L);

    uint8_t out[16];
    for(int i = 0; i < 4; i++) {
        uint16_t p = (start + i) % L->total_pages;
        mfu_read_page_masked(ctx, L, p, &out[i * 4]);
    }
    mfu_send_std(out, sizeof(out));
}

static void mfu_handle_fast_read(struct mfu_ce_context *ctx,
                                 const struct mfu_layout *L, const uint8_t *cmd, size_t len)
{
    if(len < 3) { mfu_send_nak(); return; }
    uint8_t start = cmd[1];
    uint8_t end   = cmd[2];

    if(L->total_pages == 0 || end >= L->total_pages || start > end) { mfu_send_nak(); return; }
    for(uint16_t p = start; p <= end; p++) {
        if(mfu_page_protected_read(ctx, L, p)) { mfu_send_nak(); return; }
    }
    ctx->read_cmd_count++;
    mfu_maybe_bump_counter(ctx, L);

    static uint8_t out[MFU_MAX_PAGES * 4];
    uint16_t n = (uint16_t)(end - start + 1) * 4;
    if(n > sizeof(out)) n = sizeof(out);
    for(uint8_t p = start, i = 0; p <= end && i < n; p++, i += 4)
        mfu_read_page_masked(ctx, L, p, &out[i]);
    mfu_send_std(out, n);
}

static void mfu_handle_write(struct mfu_ce_context *ctx,
                             const struct mfu_layout *L, const uint8_t *cmd, size_t len)
{
    if(len < 6) { mfu_send_nak(); return; }
    uint8_t page = cmd[1];
    if(!mfu_is_writable(ctx, L, page)) { mfu_send_nak(); return; }
    mfu_apply_write(ctx, L, page, &cmd[2]);
    mfu_send_ack();
}

static void mfu_handle_comp_write_phase1(struct mfu_ce_context *ctx,
                                         const struct mfu_layout *L,
                                         const uint8_t *cmd, size_t len)
{
    if(len < 2) { mfu_send_nak(); return; }
    uint8_t page = cmd[1];
    if(!mfu_is_writable(ctx, L, page)) { mfu_send_nak(); return; }
    ctx->in_comp_write = true;
    ctx->comp_write_page = page;
    mfu_send_ack();
}

static void mfu_handle_comp_write_phase2(struct mfu_ce_context *ctx,
                                         const struct mfu_layout *L,
                                         const uint8_t *data, size_t len)
{
    ctx->in_comp_write = false;
    if(len < 16) { mfu_send_nak(); return; }
    if(!mfu_is_writable(ctx, L, ctx->comp_write_page)) { mfu_send_nak(); return; }
    mfu_apply_write(ctx, L, ctx->comp_write_page, data);
    mfu_send_ack();
}

static void mfu_handle_get_version(struct mfu_ce_context *ctx)
{
    uint8_t ver[8];
    if(ctx->dump->version_valid) memcpy(ver, ctx->dump->version, 8);
    else                         memcpy(ver, ntag216_default_version, 8);
    mfu_send_std(ver, 8);
}

static void mfu_handle_read_sig(struct mfu_ce_context *ctx)
{
    uint8_t sig[32];
    if(ctx->dump->signature_valid) memcpy(sig, ctx->dump->signature, 32);
    else                           memset(sig, 0, 32);
    mfu_send_std(sig, sizeof(sig));
}

static void mfu_handle_pwd_auth(struct mfu_ce_context *ctx,
                                const struct mfu_layout *L, const uint8_t *cmd, size_t len)
{
    if(len < 5) { mfu_send_nak(); return; }
    if(!L->has_auth) { mfu_send_nak(); return; }

    ctx->auth_cmd_count++;
    uint8_t access = mfu_cfg1_access(ctx, L);
    uint8_t authlim = access & CFG1_AUTHLIM_MASK;
    if(authlim != 0 && ctx->authlim_counter >= authlim) {
        mfu_send_auth_nak();
        return;
    }

    const uint8_t *pwd_stored;
    const uint8_t *pack_stored;
    if(ctx->dump->auth_ok && ctx->dump->auth_kind == MFU_AUTH_PWD_NTAG) {
        pwd_stored = ctx->dump->auth_key;
        pack_stored = ctx->dump->pack;
    } else {
        pwd_stored = default_pwd;
        pack_stored = default_pack;
    }

    bool match = (memcmp(&cmd[1], pwd_stored, 4) == 0);
    mfu_record_attempt(ctx, &cmd[1], match, pack_stored);

    if(!match) {
        if(authlim != 0) ctx->authlim_counter++;
        mfu_send_auth_nak();
        return;
    }

    ctx->pwd_auth_ok = true;
    ctx->authlim_counter = 0;
    mfu_send_std(pack_stored, 2);
}

static void mfu_handle_read_cnt(struct mfu_ce_context *ctx,
                                const struct mfu_layout *L, const uint8_t *cmd, size_t len)
{
    if(len < 2) { mfu_send_nak(); return; }
    uint8_t ctr = cmd[1];
    if(!L->has_counter || ctr != 0x02) { mfu_send_nak(); return; }

    uint8_t access = mfu_cfg1_access(ctx, L);
    if((access & CFG1_NFC_CNT_PWD_MASK) && !ctx->pwd_auth_ok) {
        mfu_send_nak();
        return;
    }

    uint32_t c = ctx->dump->nfc_counter;
    uint8_t out[3] = {
        (uint8_t)(c & 0xFF),
        (uint8_t)((c >> 8) & 0xFF),
        (uint8_t)((c >> 16) & 0xFF),
    };
    mfu_send_std(out, sizeof(out));
}

static void mfu_handle_check_tearing(const uint8_t *cmd, size_t len)
{
    if(len < 2) { mfu_send_nak(); return; }
    uint8_t out = MFU_TEARING_OK;
    mfu_send_std(&out, 1);
}

static int mfu_process_command(struct mfu_ce_context *ctx,
                               const struct mfu_layout *L,
                               const uint8_t *data, size_t len)
{
    if(len == 0) return MFU_CMD_CONTINUE;

    ctx->last_rx_ms = uptime_ms();
    if(ctx->first_rx_ms == 0) ctx->first_rx_ms = ctx->last_rx_ms;

    if(ctx->in_comp_write) {
        mfu_handle_comp_write_phase2(ctx, L, data, len);
        return MFU_CMD_CONTINUE;
    }

    switch(data[0]) {
    case MFU_CMD_READ:        mfu_handle_read(ctx, L, data, len);        return MFU_CMD_CONTINUE;
    case MFU_CMD_FAST_READ:   mfu_handle_fast_read(ctx, L, data, len);   return MFU_CMD_CONTINUE;
    case MFU_CMD_WRITE:       mfu_handle_write(ctx, L, data, len);       return MFU_CMD_CONTINUE;
    case MFU_CMD_COMP_WRITE:  mfu_handle_comp_write_phase1(ctx, L, data, len); return MFU_CMD_CONTINUE;
    case MFU_CMD_GET_VERSION: mfu_handle_get_version(ctx);               return MFU_CMD_CONTINUE;
    case MFU_CMD_READ_SIG:    mfu_handle_read_sig(ctx);                  return MFU_CMD_CONTINUE;
    case MFU_CMD_PWD_AUTH:    mfu_handle_pwd_auth(ctx, L, data, len);    return MFU_CMD_CONTINUE;
    case MFU_CMD_READ_CNT:    mfu_handle_read_cnt(ctx, L, data, len);    return MFU_CMD_CONTINUE;
    case MFU_CMD_CHECK_TEAR:  mfu_handle_check_tearing(data, len);       return MFU_CMD_CONTINUE;
    case MFU_CMD_HALT:        return MFU_CMD_SLEEP;
    default:                  mfu_send_nak();                            return MFU_CMD_CONTINUE;
    }
}

static void mfu_ce_task_fn(void *arg)
{
    struct mfu_ce_context *ctx = arg;
    struct mfu_layout L;
    mfu_layout_get(ctx->dump->type, &L);
    if(L.total_pages == 0) L.total_pages = ctx->dump->total_pages;

    ce_chip_listener_init(&ctx->dump->card);
    ctx->state = CE_LISTENING;
    ESP_LOGI(TAG, "Listening %s UID-%u (auth=%d cnt=%d)",
             mfu_type_str(ctx->dump->type), ctx->dump->card.uid_len,
             (int)L.has_auth, (int)L.has_counter);

    while(atomic_load(&ctx->running)) {
        uint32_t irqs = st25r3916WaitForInterruptsTimed(
            ST25R3916_IRQ_MASK_WU_A  | ST25R3916_IRQ_MASK_WU_A_X |
            ST25R3916_IRQ_MASK_RXE_PTA |
            ST25R3916_IRQ_MASK_EOF   | ST25R3916_IRQ_MASK_EON,
            500);

        if(!atomic_load(&ctx->running)) break;

        if((irqs & (ST25R3916_IRQ_MASK_WU_A | ST25R3916_IRQ_MASK_WU_A_X)) == 0) {
            if(irqs & ST25R3916_IRQ_MASK_RXE_PTA) continue;
            if(irqs == 0 || (irqs & ST25R3916_IRQ_MASK_EOF))
                ce_chip_listener_idle();
            continue;
        }

        ce_chip_activate_pta();

        ctx->state = CE_SELECTED;
        ctx->pwd_auth_ok = false;
        ctx->in_comp_write = false;
        ctx->counter_bumped_this_activation = false;

        bool halted = false;
        while(atomic_load(&ctx->running)) {
            uint8_t buf[32];
            uint16_t rx_len = 0;
            int rxret = ce_chip_fifo_rx(buf, sizeof(buf), &rx_len, MFU_CE_RX_TIMEOUT_MS);
            if(rxret != 0 || rx_len == 0) break;

            int ret = mfu_process_command(ctx, &L, buf, rx_len);
            if(ret == MFU_CMD_SLEEP) { halted = true; break; }
            if(ret == MFU_CMD_IDLE)  { break; }
        }

        if(ctx->first_rx_ms != 0) {
            int64_t span = ctx->last_rx_ms - ctx->first_rx_ms;
            ESP_LOGI(TAG, "session: rd=%u wr=%u auth=%u span=%lldms",
                     (unsigned)ctx->read_cmd_count, (unsigned)ctx->write_cmd_count,
                     (unsigned)ctx->auth_cmd_count, (long long)span);
        }
        ctx->first_rx_ms = 0;
        ctx->last_rx_ms = 0;
        ctx->pwd_auth_ok = false;
        ctx->in_comp_write = false;
        ctx->state = CE_LISTENING;

        if(halted) ce_chip_listener_sleep();
        else       ce_chip_listener_idle();
    }

    st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    ctx->state = CE_IDLE;
    ESP_LOGI(TAG, "Stopped (rd=%u wr=%u attempts=%u)",
             (unsigned)ctx->read_cmd_count, (unsigned)ctx->write_cmd_count,
             (unsigned)ctx->attempt_count);
    atomic_store(&ctx->stopped, true);
    ctx->task = NULL;
    vTaskDelete(NULL);
}

int mfu_ce_start(struct mfu_ce_context *ctx, struct mfu_dump *dump)
{
    if(atomic_load(&ctx->running)) return -EALREADY;
    if(!dump || dump->card.uid_len == 0) return -EINVAL;

    if(st25r3916Initialize() != RFAL_ERR_NONE) return -EIO;
    nfc_chip_config();

    struct mfu_dump *dump_save = dump;
    memset(ctx, 0, sizeof(*ctx));
    ctx->dump = dump_save;
    atomic_store(&ctx->running, true);
    atomic_store(&ctx->stopped, false);
    ctx->state = CE_IDLE;

    BaseType_t ok = xTaskCreate(mfu_ce_task_fn, "mfu_emu", MFU_CE_STACK_SIZE,
                                ctx, MFU_CE_PRIORITY, &ctx->task);
    if(ok != pdPASS) {
        atomic_store(&ctx->running, false);
        ESP_LOGE(TAG, "task create failed");
        return -ENOMEM;
    }
    return 0;
}

int mfu_ce_stop(struct mfu_ce_context *ctx)
{
    if(!atomic_load(&ctx->running)) return 0;
    atomic_store(&ctx->running, false);

    const int max_ticks = pdMS_TO_TICKS(3000);
    int waited = 0;
    while(!atomic_load(&ctx->stopped) && waited < max_ticks) {
        vTaskDelay(pdMS_TO_TICKS(20));
        waited += pdMS_TO_TICKS(20);
    }
    ctx->state = CE_IDLE;
    return 0;
}

bool mfu_ce_is_running(struct mfu_ce_context *ctx)
{
    return atomic_load(&ctx->running);
}

uint8_t mfu_ce_attempt_count(const struct mfu_ce_context *ctx) { return ctx->attempt_count; }
const struct mfu_pwd_attempt *mfu_ce_attempts(const struct mfu_ce_context *ctx) { return ctx->attempts; }
void mfu_ce_attempts_clear(struct mfu_ce_context *ctx) { ctx->attempt_count = 0; }
uint32_t mfu_ce_read_count(const struct mfu_ce_context *ctx) { return ctx->read_cmd_count; }
uint32_t mfu_ce_write_count(const struct mfu_ce_context *ctx) { return ctx->write_cmd_count; }
