#include "mf_classic_listener.h"
#include "ce_chip.h"
#include "nfc_poller.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>
#include <string.h>
#include <nfc.h>

static const char *TAG = "mfc_ce";

#define CE_STACK_SIZE     4096
#define CE_PRIORITY       8
#define CE_RX_TIMEOUT_MS  20

#define MFC_CMD_AUTH_A  0x60
#define MFC_CMD_AUTH_B  0x61
#define MFC_CMD_READ    0x30
#define MFC_CMD_WRITE   0xA0
#define MFC_CMD_DEC     0xC0
#define MFC_CMD_INC     0xC1
#define MFC_CMD_REST    0xC2
#define MFC_CMD_TRANS   0xB0
#define MFC_CMD_HALT    0x50

#define MFC_ACK_NIB     0x0A
#define MFC_NACK_NIB    0x00

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))

enum ce_cmd_result {
    CE_CMD_CONTINUE = 0,
    CE_CMD_SLEEP,
    CE_CMD_IDLE,
    CE_CMD_EXPECT_WRITE,
};

static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

static inline uint64_t key_from_bytes(const uint8_t *k)
{
    uint64_t key = 0;
    for(int i = 0; i < 6; i++) key = (key << 8) | k[i];
    return key;
}

static uint64_t sector_auth_key(const struct mfc_dump *dump, int sector,
                                uint8_t key_type)
{
    uint64_t mask_bit = 1ULL << sector;
    int trailer = mfc_sector_trailer(sector);

    if(key_type == MFC_KEY_A) {
        if(dump->key_a_mask & mask_bit) return key_from_bytes(dump->key_a[sector]);
    } else {
        if(dump->key_b_mask & mask_bit) return key_from_bytes(dump->key_b[sector]);
    }

    const uint8_t *k = (key_type == MFC_KEY_A) ? &dump->blocks[trailer][0]
                                                : &dump->blocks[trailer][10];
    uint64_t key = key_from_bytes(k);
    return key ? key : 0xFFFFFFFFFFFFULL;
}

static void ce_send_short_encrypted(struct mfc_ce_context *ctx, uint8_t nibble)
{
    uint8_t byte = nibble & 0x0F;
    if(ctx->crypto_active) byte ^= crypto1_byte(&ctx->crypto, 0, 0) & 0x0F;
    ce_chip_fifo_tx(&byte, 4);
}

static const uint64_t common_keys[] = {
    0xFFFFFFFFFFFFULL, 0xA0A1A2A3A4A5ULL, 0xD3F7D3F7D3F7ULL,
    0x000000000000ULL, 0xB0B1B2B3B4B5ULL, 0x4D3A99C351DDULL,
    0x1A982C7E459AULL, 0xAABBCCDDEEFFULL, 0x714C5C886E97ULL,
    0x587EE5F9350FULL, 0xA0478CC39091ULL, 0x533CB6C723F6ULL,
    0x8FD0A4F256E9ULL,
};

static bool ce_try_key(struct crypto1_state *crypto, uint64_t key,
                       uint32_t uid32, uint32_t nT, uint32_t nR, uint32_t aR_enc)
{
    crypto1_init(crypto, key);
    crypto1_word(crypto, nT ^ uid32, 0);
    crypto1_word(crypto, nR, 1);
    uint32_t aR = aR_enc ^ crypto1_word(crypto, 0, 0);
    return aR == crypto1_prng_successor(nT, 64);
}

static int ce_handle_auth(struct mfc_ce_context *ctx, const uint8_t *cmd,
                          size_t cmd_len)
{
    if(cmd_len < 2) return -EINVAL;

    uint8_t key_type = cmd[0];
    uint8_t block = cmd[1];
    uint8_t sector = mfc_block_to_sector(block);
    uint8_t trailer = mfc_sector_trailer(sector);

    if(!ctx->dump->block_read[trailer]) return -ENOENT;

    uint32_t uid32 = bytes_to_be32(ctx->dump->card.uid);

    uint8_t nt_bytes[4];
    uint32_t nt_raw = esp_random();
    nt_bytes[0] = (nt_raw >> 24) & 0xFF;
    nt_bytes[1] = (nt_raw >> 16) & 0xFF;
    nt_bytes[2] = (nt_raw >> 8) & 0xFF;
    nt_bytes[3] = nt_raw & 0xFF;
    uint32_t nT = bytes_to_be32(nt_bytes);

    bool nested = ctx->crypto_active;

    crypto1_init(&ctx->crypto, 0xFFFFFFFFFFFFULL);
    crypto1_word(&ctx->crypto, nT ^ uid32, 0);

    if(nested) {
        uint8_t nt_enc[4];
        for(int i = 0; i < 4; i++)
            nt_enc[i] = nt_bytes[i] ^ crypto1_byte(&ctx->crypto, 0, 0);
        ce_chip_fifo_tx(nt_enc, 4 * 8);
    } else {
        ce_chip_fifo_tx(nt_bytes, 4 * 8);
    }
    ce_chip_prepare_rx();

    uint8_t rx[16];
    uint16_t rx_len = 0;
    if(ce_chip_fifo_rx(rx, sizeof(rx), &rx_len, CE_RX_TIMEOUT_MS) != 0 ||
       rx_len < 8) {
        ctx->crypto_active = false;
        return -EIO;
    }

    uint8_t *nr_ar = rx;
    if(rx_len >= 12 && memcmp(rx, nt_bytes, 4) == 0) nr_ar = rx + 4;

    uint32_t nR = bytes_to_be32(nr_ar);
    uint32_t aR_enc = bytes_to_be32(nr_ar + 4);

    uint64_t trailer_key = sector_auth_key(ctx->dump, sector, key_type);
    bool matched = ce_try_key(&ctx->crypto, trailer_key, uid32, nT, nR, aR_enc);

    if(!matched) {
        for(size_t k = 0; k < ARRAY_SIZE(common_keys); k++) {
            if(common_keys[k] == trailer_key) continue;
            if(ce_try_key(&ctx->crypto, common_keys[k], uid32, nT, nR, aR_enc)) {
                matched = true;
                break;
            }
        }
    }

    if(ctx->nonce_count < MFKEY_MAX_NONCES) {
        bool dup = false;
        for(uint8_t i = 0; i < ctx->nonce_count; i++) {
            if(ctx->nonces[i].sector == sector &&
               ctx->nonces[i].key_type == key_type &&
               ctx->nonces[i].nt == nT) { dup = true; break; }
        }
        if(!dup) {
            struct mfkey_nonce *n = &ctx->nonces[ctx->nonce_count++];
            n->uid = uid32;
            n->sector = sector;
            n->key_type = key_type;
            n->nt = nT;
            n->nr = nR;
            n->ar = aR_enc;
            ESP_LOGI(TAG, "N%u/%c nt=%08lx nr=%08lx ar=%08lx", sector,
                     key_type == MFC_KEY_A ? 'A' : 'B',
                     (unsigned long)nT, (unsigned long)nR, (unsigned long)aR_enc);
            if(ctx->nonce_cb) ctx->nonce_cb(n, ctx->nonce_cb_ctx);
        }
    }

    if(!matched) {
        ctx->crypto_active = false;
        return -EACCES;
    }

    uint32_t aT = crypto1_prng_successor(nT, 96);
    uint8_t aT_plain[4] = {
        (aT >> 24) & 0xFF, (aT >> 16) & 0xFF,
        (aT >> 8) & 0xFF,   aT       & 0xFF
    };

    uint8_t enc[4];
    for(int i = 0; i < 4; i++) enc[i] = aT_plain[i] ^ crypto1_byte(&ctx->crypto, 0, 0);

    ce_chip_fifo_tx(enc, 4 * 8);
    ce_chip_prepare_rx();

    ctx->crypto_active = true;
    ctx->auth_sector = sector;
    return 0;
}

static int ce_handle_read(struct mfc_ce_context *ctx, uint8_t block)
{
    if(!ctx->crypto_active) return -EPERM;
    if(block >= mfc_total_blocks(ctx->dump->type)) return -EINVAL;

    uint8_t plain[18];
    if(ctx->dump->block_read[block])
        memcpy(plain, ctx->dump->blocks[block], MFC_BLOCK_SIZE);
    else
        memset(plain, 0, MFC_BLOCK_SIZE);
    iso14443a_crc(plain, 16, &plain[16]);

    uint8_t enc[18];
    for(int i = 0; i < 18; i++) enc[i] = plain[i] ^ crypto1_byte(&ctx->crypto, 0, 0);

    ce_chip_fifo_tx(enc, 18 * 8);
    ce_chip_prepare_rx();

    if(ctx->read_start_ms == 0) ctx->read_start_ms = uptime_ms();
    if(block == mfc_total_blocks(ctx->dump->type) - 1) {
        int64_t elapsed = uptime_ms() - ctx->read_start_ms;
        ESP_LOGI(TAG, "read %lld ms", (long long)elapsed);
        ctx->read_start_ms = 0;
    }
    return 0;
}

static int ce_handle_write_phase1(struct mfc_ce_context *ctx, uint8_t block)
{
    if(!ctx->crypto_active) return -EPERM;
    if(block == 0 || block >= mfc_total_blocks(ctx->dump->type)) return -EPERM;
    if(mfc_block_to_sector(block) != ctx->auth_sector) return -EPERM;

    ctx->write_block = block;
    ctx->pending_write = true;
    ce_send_short_encrypted(ctx, MFC_ACK_NIB);
    ce_chip_prepare_rx();
    return 0;
}

static int ce_handle_write_phase2(struct mfc_ce_context *ctx, const uint8_t *data,
                                  size_t len)
{
    ctx->pending_write = false;
    if(!ctx->crypto_active || len < 18) return -EIO;

    uint8_t plain[18];
    for(int i = 0; i < 18; i++) plain[i] = data[i] ^ crypto1_byte(&ctx->crypto, 0, 0);

    uint8_t crc[2];
    iso14443a_crc(plain, 16, crc);
    if(crc[0] != plain[16] || crc[1] != plain[17]) {
        ce_send_short_encrypted(ctx, MFC_NACK_NIB);
        ce_chip_prepare_rx();
        return 0;
    }

    memcpy(ctx->dump->blocks[ctx->write_block], plain, MFC_BLOCK_SIZE);
    ctx->dump->block_read[ctx->write_block] = true;

    ce_send_short_encrypted(ctx, MFC_ACK_NIB);
    ce_chip_prepare_rx();
    return 0;
}

static int ce_process_command(struct mfc_ce_context *ctx, const uint8_t *data,
                              size_t len)
{
    if(len == 0) return CE_CMD_CONTINUE;

    if(ctx->pending_write) {
        uint8_t buf[18];
        size_t n = (len > sizeof(buf)) ? sizeof(buf) : len;
        memcpy(buf, data, n);
        ce_handle_write_phase2(ctx, buf, n);
        return CE_CMD_CONTINUE;
    }

    uint8_t cmd[32];
    size_t cmd_len = (len > sizeof(cmd)) ? sizeof(cmd) : len;

    if(ctx->crypto_active) {
        for(size_t i = 0; i < cmd_len; i++)
            cmd[i] = data[i] ^ crypto1_byte(&ctx->crypto, 0, 0);
    } else {
        memcpy(cmd, data, cmd_len);
    }

    switch(cmd[0]) {
    case MFC_CMD_AUTH_A:
    case MFC_CMD_AUTH_B:
        ctx->crypto_active = false;
        if(ce_handle_auth(ctx, cmd, cmd_len) != 0) return CE_CMD_SLEEP;
        return CE_CMD_CONTINUE;

    case MFC_CMD_READ:
        if(cmd_len < 2 || !ctx->crypto_active) {
            ce_send_short_encrypted(ctx, MFC_NACK_NIB);
            ce_chip_prepare_rx();
            return CE_CMD_CONTINUE;
        }
        if(ce_handle_read(ctx, cmd[1]) != 0) {
            ce_send_short_encrypted(ctx, MFC_NACK_NIB);
            ce_chip_prepare_rx();
        }
        return CE_CMD_CONTINUE;

    case MFC_CMD_WRITE:
        if(cmd_len < 2 || ce_handle_write_phase1(ctx, cmd[1]) != 0) {
            ce_send_short_encrypted(ctx, MFC_NACK_NIB);
            ce_chip_prepare_rx();
        }
        return CE_CMD_CONTINUE;

    case MFC_CMD_DEC:
    case MFC_CMD_INC:
    case MFC_CMD_REST:
    case MFC_CMD_TRANS:
        ce_send_short_encrypted(ctx, MFC_NACK_NIB);
        ce_chip_prepare_rx();
        return CE_CMD_CONTINUE;

    case MFC_CMD_HALT:  return CE_CMD_SLEEP;
    default:            return CE_CMD_SLEEP;
    }
}

static void ce_task_fn(void *arg)
{
    struct mfc_ce_context *ctx = arg;
    ce_chip_listener_init(&ctx->dump->card);

    ctx->state = CE_LISTENING;
    ESP_LOGI(TAG, "Listening (auto-ac)");

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
        ctx->crypto_active = false;
        ctx->pending_write = false;

        bool halted = false;
        while(atomic_load(&ctx->running)) {
            uint8_t buf[32];
            uint16_t rx_len = 0;
            int rxret = ce_chip_fifo_rx(buf, sizeof(buf), &rx_len, CE_RX_TIMEOUT_MS);
            if(rxret != 0 || rx_len == 0) break;

            int ret = ce_process_command(ctx, buf, rx_len);
            if(ret == CE_CMD_SLEEP) { halted = true; break; }
            if(ret == CE_CMD_IDLE)  { break; }
        }

        ctx->crypto_active = false;
        ctx->pending_write = false;
        ctx->state = CE_LISTENING;

        if(halted) ce_chip_listener_sleep();
        else       ce_chip_listener_idle();
    }

    st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    ctx->state = CE_IDLE;
    ESP_LOGI(TAG, "Stopped (nonces=%u)", (unsigned)ctx->nonce_count);
    atomic_store(&ctx->stopped, true);
    ctx->task = NULL;
    vTaskDelete(NULL);
}

int mfc_ce_start(struct mfc_ce_context *ctx, struct mfc_dump *dump,
                 mfc_ce_nonce_cb_t cb, void *cb_ctx)
{
    if(atomic_load(&ctx->running)) return -EALREADY;
    if(!dump) return -EINVAL;

    if(st25r3916Initialize() != RFAL_ERR_NONE) return -EIO;
    nfc_chip_config();

    struct mfc_dump *dump_save = dump;
    mfc_ce_nonce_cb_t cb_save = cb;
    void *cb_ctx_save = cb_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->dump = dump_save;
    ctx->nonce_cb = cb_save;
    ctx->nonce_cb_ctx = cb_ctx_save;
    atomic_store(&ctx->running, true);
    atomic_store(&ctx->stopped, false);
    ctx->state = CE_IDLE;

    BaseType_t ok = xTaskCreate(ce_task_fn, "mfc_emu", CE_STACK_SIZE,
                                ctx, CE_PRIORITY, &ctx->task);
    if(ok != pdPASS) {
        atomic_store(&ctx->running, false);
        ESP_LOGE(TAG, "task create failed");
        return -ENOMEM;
    }
    return 0;
}

int mfc_ce_stop(struct mfc_ce_context *ctx)
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
    ctx->crypto_active = false;
    return 0;
}

bool mfc_ce_is_running(struct mfc_ce_context *ctx)        { return atomic_load(&ctx->running); }
uint8_t mfc_ce_nonce_count(const struct mfc_ce_context *ctx) { return ctx->nonce_count; }
const struct mfkey_nonce *mfc_ce_nonces(const struct mfc_ce_context *ctx) { return ctx->nonces; }
void mfc_ce_nonces_clear(struct mfc_ce_context *ctx) { ctx->nonce_count = 0; }
