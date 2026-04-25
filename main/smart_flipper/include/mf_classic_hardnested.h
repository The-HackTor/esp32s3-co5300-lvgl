#ifndef MF_CLASSIC_HARDNESTED_H
#define MF_CLASSIC_HARDNESTED_H

#include "mf_classic_nested.h"
#include <stdint.h>
#include <stdbool.h>

enum hardnested_phase {
	HARDNESTED_PHASE_COLLECT,
	HARDNESTED_PHASE_ANALYZE,
	HARDNESTED_PHASE_SOLVE,
	HARDNESTED_PHASE_DONE,
};

struct hardnested_result {
	bool     found;
	uint8_t  key[6];
	uint32_t nonces_collected;
	uint32_t elapsed_ms;
};

typedef void (*hardnested_progress_cb)(enum hardnested_phase phase,
				       uint32_t nonces, void *ctx);

int hardnested_attack(const struct nested_params *params,
		      struct hardnested_result *result,
		      hardnested_progress_cb progress, void *ctx);

#endif
