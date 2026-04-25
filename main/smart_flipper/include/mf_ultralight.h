#ifndef MFU_H
#define MFU_H

#include <nfc.h>
#include <stdbool.h>

#define MFU_CMD_READ         0x30
#define MFU_CMD_WRITE        0xA2
#define MFU_CMD_GET_VERSION  0x60
#define MFU_CMD_FAST_READ    0x3A
#define MFU_CMD_PWD_AUTH     0x1B
#define MFU_CMD_READ_CNT     0x39
#define MFU_CMD_READ_SIG     0x3C
#define MFU_CMD_AUTH_3DES    0x1A
#define MFU_CMD_AF           0xAF
#define MFU_CMD_SECTOR_SEL   0xC2

#define MFU_PAGE_SIZE  4
#define MFU_MAX_PAGES  231

enum mfu_type {
	MFU_TYPE_UNKNOWN,
	MFU_TYPE_UL,
	MFU_TYPE_UL_C,
	MFU_TYPE_NTAG203,
	MFU_TYPE_UL_EV1_11,
	MFU_TYPE_UL_EV1_21,
	MFU_TYPE_UL_NANO,
	MFU_TYPE_NTAG210,
	MFU_TYPE_NTAG212,
	MFU_TYPE_NTAG213,
	MFU_TYPE_NTAG213F,
	MFU_TYPE_NTAG215,
	MFU_TYPE_NTAG216,
	MFU_TYPE_NTAG216F,
	MFU_TYPE_NTAG_I2C_1K,
	MFU_TYPE_NTAG_I2C_2K,
	MFU_TYPE_NTAG_I2C_PLUS_1K,
	MFU_TYPE_NTAG_I2C_PLUS_2K,
};

enum mfu_auth_kind {
	MFU_AUTH_NONE      = 0,
	MFU_AUTH_3DES_ULC  = 1,
	MFU_AUTH_PWD_NTAG  = 2,
};

struct mfu_dump {
	struct iso14443a_card card;
	enum mfu_type type;
	uint8_t  pages[MFU_MAX_PAGES][MFU_PAGE_SIZE];
	bool     page_read[MFU_MAX_PAGES];
	uint16_t total_pages;
	uint8_t  version[8];
	bool     version_valid;
	uint8_t  signature[32];
	bool     signature_valid;
	uint32_t read_time_ms;
	uint32_t nfc_counter;

	enum mfu_auth_kind auth_kind;
	bool     auth_ok;
	uint8_t  auth_key[16];
	uint8_t  pack[2];
};

static inline const char *mfu_type_str(enum mfu_type type)
{
	switch (type) {
	case MFU_TYPE_UL:                return "MIFARE Ultralight";
	case MFU_TYPE_UL_C:              return "MIFARE Ultralight C";
	case MFU_TYPE_NTAG203:           return "NTAG203";
	case MFU_TYPE_UL_EV1_11:         return "Ultralight EV1 (48B)";
	case MFU_TYPE_UL_EV1_21:         return "Ultralight EV1 (128B)";
	case MFU_TYPE_UL_NANO:           return "Ultralight Nano";
	case MFU_TYPE_NTAG210:           return "NTAG210";
	case MFU_TYPE_NTAG212:           return "NTAG212";
	case MFU_TYPE_NTAG213:           return "NTAG213";
	case MFU_TYPE_NTAG213F:          return "NTAG213F";
	case MFU_TYPE_NTAG215:           return "NTAG215";
	case MFU_TYPE_NTAG216:           return "NTAG216";
	case MFU_TYPE_NTAG216F:          return "NTAG216F";
	case MFU_TYPE_NTAG_I2C_1K:       return "NTAG I2C 1K";
	case MFU_TYPE_NTAG_I2C_2K:       return "NTAG I2C 2K";
	case MFU_TYPE_NTAG_I2C_PLUS_1K:  return "NTAG I2C Plus 1K";
	case MFU_TYPE_NTAG_I2C_PLUS_2K:  return "NTAG I2C Plus 2K";
	default:                         return "Unknown Ultralight";
	}
}

static inline uint16_t mfu_type_pages(enum mfu_type type)
{
	switch (type) {
	case MFU_TYPE_UL:                return 16;
	case MFU_TYPE_UL_C:              return 48;
	case MFU_TYPE_NTAG203:           return 42;
	case MFU_TYPE_UL_EV1_11:         return 20;
	case MFU_TYPE_UL_EV1_21:         return 41;
	case MFU_TYPE_UL_NANO:           return 17;
	case MFU_TYPE_NTAG210:           return 20;
	case MFU_TYPE_NTAG212:           return 41;
	case MFU_TYPE_NTAG213:           return 45;
	case MFU_TYPE_NTAG213F:          return 45;
	case MFU_TYPE_NTAG215:           return 135;
	case MFU_TYPE_NTAG216:           return 231;
	case MFU_TYPE_NTAG216F:          return 231;
	case MFU_TYPE_NTAG_I2C_1K:       return 231;
	case MFU_TYPE_NTAG_I2C_2K:       return 231;
	case MFU_TYPE_NTAG_I2C_PLUS_1K:  return 231;
	case MFU_TYPE_NTAG_I2C_PLUS_2K:  return 231;
	default:                         return 16;
	}
}

static inline bool nfc_is_mfu(const struct iso14443a_card *card)
{
	return card->sak == 0x00 &&
	       (card->atqa[0] == 0x44 || card->atqa[0] == 0x04);
}

int mfu_get_version(uint8_t *version);
int mfu_read_pages(uint8_t start, uint8_t *data, uint16_t *rx_len);
int mfu_fast_read(uint8_t start, uint8_t end, uint8_t *data, uint16_t *rx_len);
int mfu_write_page(uint8_t page, const uint8_t *data);
int mfu_read_sig(uint8_t *sig);
int mfu_read_cnt(uint8_t index, uint8_t *out_3);
int mfu_pwd_auth(const uint8_t pwd[4], uint8_t pack_out[2]);
int mfu_auth_3des(const uint8_t key[16]);
int mfu_sector_select(uint8_t sector);
int mfu_dump_card(struct mfu_dump *dump);

enum mfu_type mfu_detect_type(const uint8_t *version);

extern const uint8_t mfu_default_3des_key[16];
extern const uint8_t mfu_default_pwd[4];

#endif
