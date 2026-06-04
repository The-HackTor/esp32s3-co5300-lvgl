#include "mf_classic_poller.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "mfc_poller";

#define MAX_FOUND_KEYS 80

static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

struct sector_key_state {
    uint8_t key_a[6];
    uint8_t key_b[6];
    bool    key_a_found;
    bool    key_b_found;
};

struct poller_ctx {
    struct mfc_dump *dump;
    struct sector_key_state keys[MFC_MAX_SECTORS];
    uint8_t found_keys[MAX_FOUND_KEYS][6];
    uint8_t found_count;
    int nsectors;
    int nblocks;
    bool crypto_live;
    mfc_poller_progress_cb progress;
    void *progress_ctx;
};

static void report(struct poller_ctx *ctx, enum mfc_poller_phase phase)
{
    if(!ctx->progress) return;
    uint8_t read = 0;
    for(int s = 0; s < ctx->nsectors; s++) {
        int trailer = mfc_sector_trailer(s);
        if(ctx->dump->block_read[trailer]) read++;
    }
    ctx->progress(phase, read, ctx->nsectors, ctx->progress_ctx);
}

static void add_found_key(struct poller_ctx *ctx, const uint8_t *key)
{
    for(uint8_t i = 0; i < ctx->found_count; i++) {
        if(memcmp(ctx->found_keys[i], key, 6) == 0) return;
    }
    if(ctx->found_count < MAX_FOUND_KEYS) {
        memcpy(ctx->found_keys[ctx->found_count], key, 6);
        ctx->found_count++;
    }
}

static void record_key(struct poller_ctx *ctx, int sector,
                       uint8_t key_type, const uint8_t *key)
{
    if(key_type == MFC_KEY_A) {
        ctx->keys[sector].key_a_found = true;
        memcpy(ctx->keys[sector].key_a, key, 6);
        memcpy(ctx->dump->key_a[sector], key, 6);
        ctx->dump->key_a_mask |= 1ULL << sector;
    } else {
        ctx->keys[sector].key_b_found = true;
        memcpy(ctx->keys[sector].key_b, key, 6);
        memcpy(ctx->dump->key_b[sector], key, 6);
        ctx->dump->key_b_mask |= 1ULL << sector;
    }
}

static int do_auth(struct poller_ctx *ctx, int sector,
                   const uint8_t *key, uint8_t key_type)
{
    int trailer = mfc_sector_trailer(sector);
    const uint8_t *uid = ctx->dump->card.uid;

    if(ctx->crypto_live) {
        int ret = mfc_auth_nested(trailer, key_type, key, uid);
        if(ret == 0) return 0;
    }

    nfc_reactivate(&ctx->dump->card);
    ctx->crypto_live = false;

    int ret = mfc_auth(trailer, key_type, key, uid);
    if(ret == 0) { ctx->crypto_live = true; return 0; }
    return ret;
}

static bool read_sector(struct poller_ctx *ctx, int sector,
                        const uint8_t *key, uint8_t key_type)
{
    int first = mfc_sector_first_block(sector);
    int nblk  = mfc_sector_block_count(sector);

    for(int attempt = 0; attempt < 3; attempt++) {
        if(attempt > 0) {
            if(do_auth(ctx, sector, key, key_type) != 0) continue;
        }
        bool all_ok = true;
        for(int b = 0; b < nblk; b++) {
            int blk = first + b;
            if(ctx->dump->block_read[blk]) continue;
            if(mfc_read_block(blk, ctx->dump->blocks[blk]) == 0) {
                ctx->dump->block_read[blk] = true;
            } else {
                all_ok = false;
                break;
            }
        }
        if(all_ok) return true;
    }
    return false;
}

static void phase_backdoor(struct poller_ctx *ctx)
{
    int bd = mfc_detect_backdoor(ctx->dump->card.uid);
    if(bd < 0) return;

    ESP_LOGI(TAG, "backdoor detected: idx=%d", bd);

    for(int s = 0; s < ctx->nsectors; s++) {
        int trailer = mfc_sector_trailer(s);
        nfc_reactivate(&ctx->dump->card);
        ctx->crypto_live = false;
        if(mfc_auth(trailer, MFC_KEY_A, mfc_backdoor_keys[bd],
                    ctx->dump->card.uid) == 0) {
            ctx->crypto_live = true;
            read_sector(ctx, s, mfc_backdoor_keys[bd], MFC_KEY_A);
            record_key(ctx, s, MFC_KEY_A, mfc_backdoor_keys[bd]);
        }
    }
    report(ctx, MFC_PHASE_BACKDOOR);
}

static void phase_key_reuse(struct poller_ctx *ctx)
{
    for(uint8_t fi = 0; fi < ctx->found_count; fi++) {
        for(int s = 0; s < ctx->nsectors; s++) {
            if(ctx->keys[s].key_a_found && ctx->keys[s].key_b_found) continue;

            if(!ctx->keys[s].key_a_found) {
                if(do_auth(ctx, s, ctx->found_keys[fi], MFC_KEY_A) == 0) {
                    read_sector(ctx, s, ctx->found_keys[fi], MFC_KEY_A);
                    record_key(ctx, s, MFC_KEY_A, ctx->found_keys[fi]);
                }
            }
            if(!ctx->keys[s].key_b_found) {
                if(do_auth(ctx, s, ctx->found_keys[fi], MFC_KEY_B) == 0) {
                    read_sector(ctx, s, ctx->found_keys[fi], MFC_KEY_B);
                    record_key(ctx, s, MFC_KEY_B, ctx->found_keys[fi]);
                }
            }
        }
    }
    report(ctx, MFC_PHASE_KEY_REUSE);
}

static void phase_dict_attack(struct poller_ctx *ctx)
{
    for(int s = 0; s < ctx->nsectors; s++) {
        int trailer = mfc_sector_trailer(s);
        if(ctx->dump->block_read[trailer]) continue;

        for(size_t ki = 0; ki < mfc_default_keys_count; ki++) {
            uint8_t kt = MFC_KEY_A;
            if(do_auth(ctx, s, mfc_default_keys[ki], MFC_KEY_A) != 0) {
                if(do_auth(ctx, s, mfc_default_keys[ki], MFC_KEY_B) != 0) continue;
                kt = MFC_KEY_B;
            }
            add_found_key(ctx, mfc_default_keys[ki]);
            read_sector(ctx, s, mfc_default_keys[ki], kt);
            record_key(ctx, s, kt, mfc_default_keys[ki]);
            phase_key_reuse(ctx);
            break;
        }

        if((s % 4) == 0) report(ctx, MFC_PHASE_DICT_ATTACK);
    }
    report(ctx, MFC_PHASE_DICT_ATTACK);
}

static void phase_read_remaining(struct poller_ctx *ctx)
{
    for(int s = 0; s < ctx->nsectors; s++) {
        int trailer = mfc_sector_trailer(s);
        if(ctx->dump->block_read[trailer]) continue;

        const uint8_t *key = NULL;
        uint8_t kt = MFC_KEY_A;
        if(ctx->keys[s].key_a_found) { key = ctx->keys[s].key_a; kt = MFC_KEY_A; }
        else if(ctx->keys[s].key_b_found) { key = ctx->keys[s].key_b; kt = MFC_KEY_B; }
        if(!key) continue;

        if(do_auth(ctx, s, key, kt) == 0) read_sector(ctx, s, key, kt);
    }
    report(ctx, MFC_PHASE_READ);
}

int mfc_poller_dump(struct mfc_dump *dump,
                    mfc_poller_progress_cb progress, void *ctx_arg)
{
    struct poller_ctx pc;
    memset(&pc, 0, sizeof(pc));
    pc.dump = dump;
    pc.progress = progress;
    pc.progress_ctx = ctx_arg;

    int ret = nfc_detect_card(&dump->card);
    if(ret) return ret;

    dump->type = mfc_detect_type(dump->card.sak);
    if(dump->type == MFC_TYPE_UNKNOWN) return -ENOTSUP;

    pc.nsectors = mfc_total_sectors(dump->type);
    pc.nblocks  = mfc_total_blocks(dump->type);
    pc.crypto_live = true;

    memset(dump->block_read, 0, pc.nblocks * sizeof(bool));
    dump->key_a_mask = 0;
    dump->key_b_mask = 0;
    memset(dump->key_a, 0, sizeof(dump->key_a));
    memset(dump->key_b, 0, sizeof(dump->key_b));
    int64_t t_start = uptime_ms();

    report(&pc, MFC_PHASE_DETECT);

    phase_backdoor(&pc);
    bool all_done = true;
    for(int s = 0; s < pc.nsectors; s++) {
        if(!dump->block_read[mfc_sector_trailer(s)]) { all_done = false; break; }
    }
    if(all_done) goto done;

    phase_dict_attack(&pc);
    all_done = true;
    for(int s = 0; s < pc.nsectors; s++) {
        if(!dump->block_read[mfc_sector_trailer(s)]) { all_done = false; break; }
    }
    if(all_done) goto done;

    phase_read_remaining(&pc);

done:
    dump->read_time_ms = (uint32_t)(uptime_ms() - t_start);

    int read_count = 0;
    for(int s = 0; s < pc.nsectors; s++) {
        if(dump->block_read[mfc_sector_trailer(s)]) read_count++;
    }
    ESP_LOGI(TAG, "dump complete: %d/%d sectors in %u ms",
             read_count, pc.nsectors, (unsigned)dump->read_time_ms);

    report(&pc, MFC_PHASE_DONE);
    return 0;
}
