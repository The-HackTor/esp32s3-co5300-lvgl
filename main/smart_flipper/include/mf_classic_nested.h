#ifndef MFC_NESTED_H
#define MFC_NESTED_H

#include <stdint.h>
#include <stdbool.h>

enum nested_phase {
	NESTED_PHASE_DETECT_PRNG,
	NESTED_PHASE_CALIBRATE,
	NESTED_PHASE_COLLECT,
	NESTED_PHASE_SOLVE,
	NESTED_PHASE_DONE,
};

enum nested_prng_type {
	NESTED_PRNG_UNKNOWN,
	NESTED_PRNG_WEAK,
	NESTED_PRNG_HARD,
};

struct nested_params {
	uint32_t uid;
	uint8_t  known_sector;
	uint8_t  known_key_type;
	uint8_t  known_key[6];
	uint8_t  target_sector;
	uint8_t  target_key_type;
};

struct nested_result {
	bool     found;
	uint8_t  key[6];
	enum nested_prng_type prng_type;
	uint32_t elapsed_ms;
};

typedef void (*nested_progress_cb)(enum nested_phase phase, uint8_t pct,
				   void *ctx);

int nested_detect_prng(const uint8_t *uid, uint8_t known_sector,
		       uint8_t known_key_type, const uint8_t *known_key,
		       enum nested_prng_type *prng_type);

int nested_attack(const struct nested_params *params,
		  struct nested_result *result,
		  nested_progress_cb progress, void *ctx);

#endif
