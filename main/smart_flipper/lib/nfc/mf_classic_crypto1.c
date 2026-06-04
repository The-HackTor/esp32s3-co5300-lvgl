#include <nfc.h>
#include <string.h>

#define LF_POLY_ODD  (0x29CE5C)
#define LF_POLY_EVEN (0x870804)

#undef BIT
#define BIT(x, n)    (((x) >> (n)) & 1)
#define BEBIT(x, n)  BIT(x, (n) ^ 24)

static inline uint32_t even_parity32(uint32_t x) { return __builtin_parity(x); }
static inline uint8_t  even_parity8(uint8_t x)   { return __builtin_parity(x); }

static uint32_t crypto1_filter(uint32_t in)
{
    uint32_t out = 0;
    out  = 0xf22c0 >> (in        & 0xf) & 16;
    out |= 0x6c9c0 >> (in >>  4  & 0xf) & 8;
    out |= 0x3c8b0 >> (in >>  8  & 0xf) & 4;
    out |= 0x1e458 >> (in >> 12  & 0xf) & 2;
    out |= 0x0d938 >> (in >> 16  & 0xf) & 1;
    return BIT(0xEC57E80A, out);
}

void crypto1_init(struct crypto1_state *s, uint64_t key)
{
    s->odd = s->even = 0;
    for(int8_t i = 47; i > 0; i -= 2) {
        s->odd  = s->odd  << 1 | BIT(key, (i - 1) ^ 7);
        s->even = s->even << 1 | BIT(key, i ^ 7);
    }
}

void crypto1_destroy(struct crypto1_state *s) { memset(s, 0, sizeof(*s)); }

uint8_t crypto1_bit(struct crypto1_state *s, uint8_t in, int is_encrypted)
{
    uint8_t out = crypto1_filter(s->odd);
    uint32_t feed = out & (!!is_encrypted);
    feed ^= !!in;
    feed ^= LF_POLY_ODD & s->odd;
    feed ^= LF_POLY_EVEN & s->even;
    s->even = s->even << 1 | even_parity32(feed);
    uint32_t tmp = s->odd;
    s->odd = s->even;
    s->even = tmp;
    return out;
}

uint8_t crypto1_byte(struct crypto1_state *s, uint8_t in, int is_encrypted)
{
    uint8_t out = 0;
    for(int i = 0; i < 8; i++)
        out |= crypto1_bit(s, BIT(in, i), is_encrypted) << i;
    return out;
}

uint32_t crypto1_word(struct crypto1_state *s, uint32_t in, int is_encrypted)
{
    uint32_t out = 0;
    for(int i = 0; i < 32; i++)
        out |= (uint32_t)crypto1_bit(s, BEBIT(in, i), is_encrypted) << (24 ^ i);
    return out;
}

uint8_t crypto1_parity_bit(struct crypto1_state *s) { return crypto1_filter(s->odd); }

uint8_t crypto1_lfsr_rollback_bit(struct crypto1_state *s, uint8_t in, int fb)
{
    uint8_t ret;
    uint32_t t, out;
    s->odd &= 0xffffff;
    t = s->odd;
    s->odd = s->even;
    s->even = t;
    out = s->even & 1;
    out ^= LF_POLY_EVEN & (s->even >>= 1);
    out ^= LF_POLY_ODD & s->odd;
    out ^= !!in;
    out ^= (ret = crypto1_filter(s->odd)) & (!!fb);
    s->even |= even_parity32(out) << 23;
    return ret;
}

uint32_t crypto1_lfsr_rollback_word(struct crypto1_state *s, uint32_t in, int fb)
{
    uint32_t ret = 0;
    for(int i = 31; i >= 0; i--)
        ret |= (uint32_t)crypto1_lfsr_rollback_bit(s, BEBIT(in, i), fb) << (24 ^ i);
    return ret;
}

uint32_t crypto1_prng_successor(uint32_t x, uint32_t n)
{
    x = __builtin_bswap32(x);
    while(n--)
        x = (x >> 1) |
            (((x >> 16) ^ (x >> 18) ^ (x >> 19) ^ (x >> 21)) & 1) << 31;
    return __builtin_bswap32(x);
}

bool crypto1_is_weak_prng_nonce(uint32_t nonce)
{
    if(nonce == 0) return false;
    uint16_t x = nonce >> 16;
    x = (x & 0xff) << 8 | x >> 8;
    for(uint8_t i = 0; i < 16; i++)
        x = x >> 1 | (x ^ x >> 2 ^ x >> 3 ^ x >> 5) << 15;
    x = (x & 0xff) << 8 | x >> 8;
    return x == (nonce & 0xFFFF);
}

bool crypto1_nonce_matches_encrypted_parity(uint32_t nt, uint32_t ks,
                                            uint8_t par_enc)
{
    return (even_parity8((nt >> 24) & 0xFF) ==
            (((par_enc >> 3) & 1) ^ BIT(ks, 16))) &&
           (even_parity8((nt >> 16) & 0xFF) ==
            (((par_enc >> 2) & 1) ^ BIT(ks, 8))) &&
           (even_parity8((nt >> 8) & 0xFF) ==
            (((par_enc >> 1) & 1) ^ BIT(ks, 0)));
}

uint32_t crypto1_decrypt_nt_enc(uint32_t cuid, uint32_t nt_enc, uint64_t key)
{
    struct crypto1_state tmp;
    crypto1_init(&tmp, key);
    crypto1_word(&tmp, nt_enc ^ cuid, 1);
    return nt_enc ^ crypto1_lfsr_rollback_word(&tmp, nt_enc ^ cuid, 1);
}
