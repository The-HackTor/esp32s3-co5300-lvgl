#ifndef APP_NFC_EMULATION_H
#define APP_NFC_EMULATION_H

#include <stdint.h>
#include <nfc.h>

#define MFKEY_MAX_NONCES 64

struct mfkey_nonce {
	uint32_t uid;
	uint32_t nt;
	uint32_t nr;
	uint32_t ar;
	uint8_t  sector;
	uint8_t  key_type;
};

enum ce_state {
	CE_IDLE,
	CE_LISTENING,
	CE_SELECTED,
};

#endif
