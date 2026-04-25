#ifndef APP_NFC_H
#define APP_NFC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Zephyr device-tree spec structs forward-declared; UI code never derefs them. */
struct spi_dt_spec;
struct gpio_dt_spec;
struct device;

#define ISO14443A_MAX_UID_LEN  10
#define ISO14443A_SAK_MFC_1K   0x08
#define ISO14443A_SAK_MFC_4K   0x18
#define ISO14443A_SAK_CASCADE  0x04

struct iso14443a_card {
	uint8_t uid[ISO14443A_MAX_UID_LEN];
	uint8_t uid_len;
	uint8_t sak;
	uint8_t atqa[2];
};

int nfc_init(const struct spi_dt_spec *spi, const struct gpio_dt_spec *irq);
int nfc_reinit(void);
int nfc_field_on(void);
int nfc_field_off(void);
void nfc_diag(char *buf, size_t len);

int nfc_detect_card(struct iso14443a_card *card);
int nfc_reactivate(struct iso14443a_card *card);
int nfc_halt(void);

static inline uint32_t bytes_to_be32(const uint8_t *b)
{
	return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
	       ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

static inline uint64_t key_to_uint64(const uint8_t *key)
{
	uint64_t k = 0;
	for (int i = 0; i < 6; i++)
		k = (k << 8) | key[i];
	return k;
}

static inline void uint64_to_key(uint64_t k, uint8_t *key)
{
	for (int i = 5; i >= 0; i--) {
		key[i] = k & 0xFF;
		k >>= 8;
	}
}

void iso14443a_crc(const uint8_t *data, size_t len, uint8_t *crc);

struct crypto1_state {
	uint32_t odd;
	uint32_t even;
};

void     crypto1_init(struct crypto1_state *s, uint64_t key);
void     crypto1_destroy(struct crypto1_state *s);
uint8_t  crypto1_bit(struct crypto1_state *s, uint8_t in, int is_encrypted);
uint8_t  crypto1_byte(struct crypto1_state *s, uint8_t in, int is_encrypted);
uint32_t crypto1_word(struct crypto1_state *s, uint32_t in, int is_encrypted);
uint8_t  crypto1_parity_bit(struct crypto1_state *s);

uint8_t  crypto1_lfsr_rollback_bit(struct crypto1_state *s, uint8_t in, int fb);
uint32_t crypto1_lfsr_rollback_word(struct crypto1_state *s, uint32_t in, int fb);

uint32_t crypto1_prng_successor(uint32_t x, uint32_t n);
bool     crypto1_is_weak_prng_nonce(uint32_t nonce);

bool     crypto1_nonce_matches_encrypted_parity(uint32_t nt, uint32_t ks,
						uint8_t par_enc);
uint32_t crypto1_decrypt_nt_enc(uint32_t cuid, uint32_t nt_enc, uint64_t key);

#define MFC_BLOCK_SIZE      16
#define MFC_KEY_A  0x60
#define MFC_KEY_B  0x61

#define MFC_BLOCKS_PER_SEC  4
#define MFC_SECTORS_1K      16
#define MFC_TOTAL_BLOCKS    MFC_1K_BLOCKS
#define MFC_DUMP_SIZE       (MFC_MAX_BLOCKS * MFC_BLOCK_SIZE)

#define MFC_1K_SECTORS   16
#define MFC_1K_BLOCKS    64
#define MFC_4K_SECTORS   40
#define MFC_4K_BLOCKS    256
#define MFC_MAX_SECTORS  40
#define MFC_MAX_BLOCKS   256

typedef enum {
	MFC_TYPE_1K,
	MFC_TYPE_4K,
	MFC_TYPE_UNKNOWN,
} mfc_type_t;

static inline mfc_type_t mfc_detect_type(uint8_t sak)
{
	if (sak == 0x08 || sak == 0x09) return MFC_TYPE_1K;
	if (sak == 0x18) return MFC_TYPE_4K;
	return MFC_TYPE_UNKNOWN;
}

static inline int mfc_total_sectors(mfc_type_t type)
{
	switch (type) {
	case MFC_TYPE_1K:  return 16;
	case MFC_TYPE_4K:  return 40;
	default:           return 16;
	}
}

static inline int mfc_total_blocks(mfc_type_t type)
{
	switch (type) {
	case MFC_TYPE_1K:  return 64;
	case MFC_TYPE_4K:  return 256;
	default:           return 64;
	}
}

static inline int mfc_sector_first_block(int sector)
{
	if (sector < 32) return sector * 4;
	return 128 + (sector - 32) * 16;
}

static inline int mfc_sector_block_count(int sector)
{
	return (sector < 32) ? 4 : 16;
}

static inline int mfc_sector_trailer(int sector)
{
	return mfc_sector_first_block(sector) + mfc_sector_block_count(sector) - 1;
}

static inline int mfc_block_to_sector(int block)
{
	if (block < 128) return block / 4;
	return 32 + (block - 128) / 16;
}

static inline const char *mfc_type_str(mfc_type_t type)
{
	switch (type) {
	case MFC_TYPE_1K:  return "MIFARE Classic 1K";
	case MFC_TYPE_4K:  return "MIFARE Classic 4K";
	default:           return "MIFARE Classic";
	}
}

struct mfc_dump {
	struct iso14443a_card card;
	mfc_type_t type;
	uint8_t  blocks[MFC_MAX_BLOCKS][MFC_BLOCK_SIZE];
	bool     block_read[MFC_MAX_BLOCKS];
	uint64_t key_a_mask;
	uint64_t key_b_mask;
	uint8_t  key_a[MFC_MAX_SECTORS][6];
	uint8_t  key_b[MFC_MAX_SECTORS][6];
	uint32_t read_time_ms;
};

static inline bool mfc_key_a_found(const struct mfc_dump *d, int sector)
{
	return (d->key_a_mask >> sector) & 1ULL;
}

static inline bool mfc_key_b_found(const struct mfc_dump *d, int sector)
{
	return (d->key_b_mask >> sector) & 1ULL;
}

static inline int mfc_keys_found(const struct mfc_dump *d)
{
	return __builtin_popcountll(d->key_a_mask) +
	       __builtin_popcountll(d->key_b_mask);
}

static inline int mfc_sectors_read(const struct mfc_dump *d)
{
	int total = mfc_total_sectors(d->type);
	int n = 0;
	for (int s = 0; s < total; s++) {
		int first = mfc_sector_first_block(s);
		int nblk = mfc_sector_block_count(s);
		for (int b = 0; b < nblk; b++) {
			if (d->block_read[first + b]) { n++; break; }
		}
	}
	return n;
}

extern const uint8_t mfc_default_keys[][6];
extern const size_t  mfc_default_keys_count;

int mfc_auth(uint8_t block, uint8_t key_type,
	     const uint8_t *key, const uint8_t *uid);
int mfc_auth_nested(uint8_t block, uint8_t key_type,
		    const uint8_t *key, const uint8_t *uid);

struct mfc_nested_nonce_raw {
	uint32_t nt_enc;
	uint8_t  par_enc;
};

int mfc_collect_nested_nonce(uint8_t block, uint8_t key_type,
			     const uint8_t *uid,
			     struct mfc_nested_nonce_raw *out);

int mfc_read_block(uint8_t block, uint8_t *data);
int mfc_write_block(uint8_t block, const uint8_t *data);
int mfc_dump_card(struct mfc_dump *dump);
int mfc_detect_backdoor(const uint8_t *uid);

#endif
