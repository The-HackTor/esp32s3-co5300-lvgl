#include <nfc.h>
#include "nfc_trx.h"
#include "mf_classic_poller.h"

#include "esp_log.h"
#include "esp_random.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "mfc";

#define MFC_TRX_TIMEOUT_MS  100

#define MFC_BACKDOOR_AUTH_A 0x64
#define MFC_BACKDOOR_AUTH_B 0x65

#include "mf_classic_keys.inc"

const uint8_t mfc_backdoor_keys[][6] = {
    {0xA3, 0x96, 0xEF, 0xA4, 0xE2, 0x4F},
    {0xA3, 0x16, 0x67, 0xA8, 0xCE, 0xC1},
    {0x51, 0x8B, 0x33, 0x54, 0xE7, 0x60},
};
#define MFC_BACKDOOR_KEY_COUNT 3

static struct crypto1_state crypto_state;
static bool crypto_active;

void iso14443a_crc(const uint8_t *data, size_t len, uint8_t *crc)
{
    uint16_t w = 0x6363;
    for(size_t i = 0; i < len; i++) {
        uint8_t bt = data[i] ^ (uint8_t)(w & 0xFF);
        bt = bt ^ (bt << 4);
        w = (w >> 8) ^ ((uint16_t)bt << 8) ^ ((uint16_t)bt << 3) ^ ((uint16_t)bt >> 4);
    }
    crc[0] = w & 0xFF;
    crc[1] = (w >> 8) & 0xFF;
}

static inline uint8_t odd_parity8(uint8_t x) { return !__builtin_parity(x); }

static uint16_t pack_with_parity(const uint8_t *data, const uint8_t *par,
                                 uint8_t count, uint8_t *out)
{
    uint16_t pos = 0;
    memset(out, 0, (count * 9 + 7) / 8);
    for(uint8_t i = 0; i < count; i++) {
        if(pos % 8 == 0) {
            out[pos / 8] = data[i];
            pos += 8;
            if(par[i] & 1) out[pos / 8] |= 1;
            pos++;
        } else {
            uint8_t shift = pos % 8;
            out[pos / 8] |= data[i] << shift;
            out[pos / 8 + 1] = data[i] >> (8 - shift);
            if(par[i] & 1) out[pos / 8 + 1] |= 1 << shift;
            pos += 9;
        }
    }
    return pos;
}

static int mfc_transceive(const uint8_t *tx, uint16_t tx_len,
                          uint8_t *rx, uint16_t rx_size, uint16_t *rx_len)
{
    return nfc_trx(tx, tx_len, rx, rx_size, rx_len,
                   MFC_TRX_TIMEOUT_MS, true, false);
}

static int mfc_transceive_raw(uint8_t *tx, uint16_t tx_bits,
                              uint8_t *rx, uint16_t rx_size, uint16_t *rx_len)
{
    return nfc_trx_custom_parity(tx, tx_bits, rx, rx_size, rx_len, MFC_TRX_TIMEOUT_MS);
}

int mfc_auth(uint8_t block, uint8_t key_type, const uint8_t *key, const uint8_t *uid)
{
    uint8_t rx[16];
    uint16_t rx_len = 0;

    crypto_active = false;

    uint8_t auth_cmd[2] = { key_type, block };
    int ret = mfc_transceive(auth_cmd, sizeof(auth_cmd), rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) return -EIO;

    uint32_t nT = bytes_to_be32(rx);
    uint32_t uid32 = bytes_to_be32(uid);

    crypto1_init(&crypto_state, key_to_uint64(key));
    crypto1_word(&crypto_state, uid32 ^ nT, 0);

    uint8_t nR_bytes[4];
    uint32_t nR_raw = esp_random();
    nR_bytes[0] = (nR_raw >> 0) & 0xFF;
    nR_bytes[1] = (nR_raw >> 8) & 0xFF;
    nR_bytes[2] = (nR_raw >> 16) & 0xFF;
    nR_bytes[3] = (nR_raw >> 24) & 0xFF;

    uint8_t enc_data[8], enc_par[8];
    for(int i = 0; i < 4; i++) {
        enc_data[i] = nR_bytes[i] ^ crypto1_byte(&crypto_state, nR_bytes[i], 0);
        enc_par[i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(nR_bytes[i]);
    }

    uint32_t ar_state = crypto1_prng_successor(nT, 32);
    for(int i = 0; i < 4; i++) {
        ar_state = crypto1_prng_successor(ar_state, 8);
        uint8_t ar_byte = (uint8_t)ar_state;
        enc_data[4 + i] = ar_byte ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[4 + i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(ar_byte);
    }

    uint8_t packed[12];
    uint16_t tx_bits = pack_with_parity(enc_data, enc_par, 8, packed);

    rx_len = 0;
    ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) {
        ESP_LOGD(TAG, "auth2 fail: ret=%d rxl=%u", ret, rx_len);
        return -EACCES;
    }

    uint32_t aT_expected = crypto1_prng_successor(nT, 96);
    uint8_t aT_dec[4];
    for(int i = 0; i < 4; i++)
        aT_dec[i] = rx[i] ^ crypto1_byte(&crypto_state, 0, 0);

    uint32_t aT_got = bytes_to_be32(aT_dec);
    if(aT_got != aT_expected) {
        ESP_LOGD(TAG, "aT mismatch: got=%08lx exp=%08lx",
                 (unsigned long)aT_got, (unsigned long)aT_expected);
        return -EACCES;
    }

    ESP_LOGD(TAG, "AUTH OK blk=%u", block);
    crypto_active = true;
    return 0;
}

int mfc_auth_nested(uint8_t block, uint8_t key_type,
                    const uint8_t *key, const uint8_t *uid)
{
    if(!crypto_active) return -EPERM;

    uint8_t plain_cmd[4] = { key_type, block };
    iso14443a_crc(plain_cmd, 2, &plain_cmd[2]);

    uint8_t enc_data[4], enc_par[4];
    for(int i = 0; i < 4; i++) {
        enc_data[i] = plain_cmd[i] ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(plain_cmd[i]);
    }

    uint8_t packed[6];
    uint16_t tx_bits = pack_with_parity(enc_data, enc_par, 4, packed);

    uint8_t rx[16];
    uint16_t rx_len = 0;
    int ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) { crypto_active = false; return -EIO; }

    uint32_t nt_enc = bytes_to_be32(rx);
    uint32_t uid32 = bytes_to_be32(uid);

    crypto1_init(&crypto_state, key_to_uint64(key));
    uint32_t nT = crypto1_word(&crypto_state, nt_enc ^ uid32, 1) ^ nt_enc;

    uint8_t nR_bytes[4];
    uint32_t nR_raw = esp_random();
    nR_bytes[0] = (nR_raw >> 0) & 0xFF;
    nR_bytes[1] = (nR_raw >> 8) & 0xFF;
    nR_bytes[2] = (nR_raw >> 16) & 0xFF;
    nR_bytes[3] = (nR_raw >> 24) & 0xFF;

    uint8_t enc2[8], par2[8];
    for(int i = 0; i < 4; i++) {
        enc2[i] = nR_bytes[i] ^ crypto1_byte(&crypto_state, nR_bytes[i], 0);
        par2[i] = crypto1_parity_bit(&crypto_state) ^ odd_parity8(nR_bytes[i]);
    }

    uint32_t ar_state = crypto1_prng_successor(nT, 32);
    for(int i = 0; i < 4; i++) {
        ar_state = crypto1_prng_successor(ar_state, 8);
        uint8_t ar_byte = (uint8_t)ar_state;
        enc2[4 + i] = ar_byte ^ crypto1_byte(&crypto_state, 0, 0);
        par2[4 + i] = crypto1_parity_bit(&crypto_state) ^ odd_parity8(ar_byte);
    }

    uint8_t packed2[12];
    tx_bits = pack_with_parity(enc2, par2, 8, packed2);

    rx_len = 0;
    ret = mfc_transceive_raw(packed2, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) { crypto_active = false; return -EACCES; }

    uint32_t aT_expected = crypto1_prng_successor(nT, 96);
    uint8_t aT_dec[4];
    for(int i = 0; i < 4; i++)
        aT_dec[i] = rx[i] ^ crypto1_byte(&crypto_state, 0, 0);

    if(bytes_to_be32(aT_dec) != aT_expected) { crypto_active = false; return -EACCES; }
    return 0;
}

int mfc_collect_nested_nonce(uint8_t block, uint8_t key_type,
                             const uint8_t *uid,
                             struct mfc_nested_nonce_raw *out)
{
    (void)uid;
    if(!crypto_active) return -EPERM;

    uint8_t plain_cmd[4] = { key_type, block };
    iso14443a_crc(plain_cmd, 2, &plain_cmd[2]);

    uint8_t enc_data[4], enc_par[4];
    for(int i = 0; i < 4; i++) {
        enc_data[i] = plain_cmd[i] ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(plain_cmd[i]);
    }

    uint8_t packed[6];
    uint16_t tx_bits = pack_with_parity(enc_data, enc_par, 4, packed);

    uint8_t rx[16];
    uint16_t rx_len = 0;
    int ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) { crypto_active = false; return -EIO; }

    out->nt_enc = bytes_to_be32(rx);
    out->par_enc = 0;
    for(int i = 0; i < 4; i++)
        out->par_enc |= ((rx[i] >> 7) & 1) << (3 - i);

    crypto_active = false;
    return 0;
}

int mfc_read_block(uint8_t block, uint8_t *data)
{
    if(!crypto_active) return -EPERM;

    uint8_t plain_cmd[4] = { 0x30, block };
    iso14443a_crc(plain_cmd, 2, &plain_cmd[2]);

    uint8_t enc_data[4], enc_par[4];
    for(int i = 0; i < 4; i++) {
        enc_data[i] = plain_cmd[i] ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(plain_cmd[i]);
    }

    uint8_t packed[6];
    uint16_t tx_bits = pack_with_parity(enc_data, enc_par, 4, packed);

    uint8_t rx[32];
    uint16_t rx_len = 0;
    int ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 18) {
        ESP_LOGD(TAG, "read blk %u fail: ret=%d rxl=%u", block, ret, rx_len);
        return -EIO;
    }

    uint8_t dec[18];
    for(int i = 0; i < 18; i++)
        dec[i] = rx[i] ^ crypto1_byte(&crypto_state, 0, 0);

    uint8_t crc[2];
    iso14443a_crc(dec, 16, crc);
    if(crc[0] != dec[16] || crc[1] != dec[17]) {
        ESP_LOGD(TAG, "read blk %u CRC fail", block);
        return -EIO;
    }

    memcpy(data, dec, 16);
    return 0;
}

int mfc_write_block(uint8_t block, const uint8_t *data)
{
    if(!crypto_active) return -EPERM;

    uint8_t plain_cmd[4] = { 0xA0, block };
    iso14443a_crc(plain_cmd, 2, &plain_cmd[2]);

    uint8_t enc_cmd[4], enc_par[4];
    for(int i = 0; i < 4; i++) {
        enc_cmd[i] = plain_cmd[i] ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[i] = crypto1_parity_bit(&crypto_state) ^ odd_parity8(plain_cmd[i]);
    }

    uint8_t packed[6];
    uint16_t tx_bits = pack_with_parity(enc_cmd, enc_par, 4, packed);

    uint8_t rx[8];
    uint16_t rx_len = 0;
    int ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0) return -EIO;

    uint8_t plain_data[18];
    memcpy(plain_data, data, 16);
    iso14443a_crc(plain_data, 16, &plain_data[16]);

    uint8_t enc_data[18], data_par[18];
    for(int i = 0; i < 18; i++) {
        enc_data[i] = plain_data[i] ^ crypto1_byte(&crypto_state, 0, 0);
        data_par[i] = crypto1_parity_bit(&crypto_state) ^ odd_parity8(plain_data[i]);
    }

    uint8_t packed2[22];
    tx_bits = pack_with_parity(enc_data, data_par, 18, packed2);

    rx_len = 0;
    ret = mfc_transceive_raw(packed2, tx_bits, rx, sizeof(rx), &rx_len);
    return (ret < 0) ? -EIO : 0;
}

static int mfc_auth_backdoor(uint8_t block, uint8_t key_type,
                             const uint8_t *uid, int bd_idx)
{
    uint8_t rx[16];
    uint16_t rx_len = 0;

    crypto_active = false;

    uint8_t bd_cmd = (key_type == MFC_KEY_A) ? MFC_BACKDOOR_AUTH_A : MFC_BACKDOOR_AUTH_B;
    uint8_t auth_cmd[2] = { bd_cmd, block };

    int ret = mfc_transceive(auth_cmd, sizeof(auth_cmd), rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) return -EIO;

    uint32_t nT = bytes_to_be32(rx);
    uint32_t uid32 = bytes_to_be32(uid);
    const uint8_t *key = mfc_backdoor_keys[bd_idx];

    crypto1_init(&crypto_state, key_to_uint64(key));
    crypto1_word(&crypto_state, uid32 ^ nT, 0);

    uint8_t nR_bytes[4];
    uint32_t nR_raw = esp_random();
    nR_bytes[0] = (nR_raw >> 0) & 0xFF;
    nR_bytes[1] = (nR_raw >> 8) & 0xFF;
    nR_bytes[2] = (nR_raw >> 16) & 0xFF;
    nR_bytes[3] = (nR_raw >> 24) & 0xFF;

    uint8_t enc_data[8], enc_par[8];
    for(int i = 0; i < 4; i++) {
        enc_data[i] = nR_bytes[i] ^ crypto1_byte(&crypto_state, nR_bytes[i], 0);
        enc_par[i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(nR_bytes[i]);
    }

    uint32_t ar_state = crypto1_prng_successor(nT, 32);
    for(int i = 0; i < 4; i++) {
        ar_state = crypto1_prng_successor(ar_state, 8);
        uint8_t ar_byte = (uint8_t)ar_state;
        enc_data[4 + i] = ar_byte ^ crypto1_byte(&crypto_state, 0, 0);
        enc_par[4 + i]  = crypto1_parity_bit(&crypto_state) ^ odd_parity8(ar_byte);
    }

    uint8_t packed[12];
    uint16_t tx_bits = pack_with_parity(enc_data, enc_par, 8, packed);

    rx_len = 0;
    ret = mfc_transceive_raw(packed, tx_bits, rx, sizeof(rx), &rx_len);
    if(ret < 0 || rx_len < 4) return -EACCES;

    uint32_t aT_expected = crypto1_prng_successor(nT, 96);
    uint8_t aT_dec[4];
    for(int i = 0; i < 4; i++)
        aT_dec[i] = rx[i] ^ crypto1_byte(&crypto_state, 0, 0);

    if(bytes_to_be32(aT_dec) != aT_expected) return -EACCES;

    crypto_active = true;
    return 0;
}

int mfc_detect_backdoor(const uint8_t *uid)
{
    for(int i = 0; i < MFC_BACKDOOR_KEY_COUNT; i++) {
        if(mfc_auth_backdoor(0, MFC_KEY_A, uid, i) == 0) {
            crypto_active = false;
            nfc_reactivate(NULL);
            ESP_LOGI(TAG, "backdoor detected: idx=%d", i);
            return i;
        }
        nfc_reactivate(NULL);
    }
    return -1;
}

int mfc_dump_card(struct mfc_dump *dump)
{
    return mfc_poller_dump(dump, NULL, NULL);
}
