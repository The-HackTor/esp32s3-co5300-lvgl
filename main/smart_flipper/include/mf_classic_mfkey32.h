#ifndef MFKEY32_H
#define MFKEY32_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct mfkey32_nonce_pair {
	uint32_t uid;
	uint32_t nt0, nr0, ar0;
	uint32_t nt1, nr1, ar1;
};

struct mfkey32_result {
	bool     found;
	uint8_t  key[6];
	uint32_t elapsed_ms;
};

int mfkey32_solve(const struct mfkey32_nonce_pair *pair,
		  struct mfkey32_result *result);

int mfkey32_solve_batch(const struct mfkey32_nonce_pair *pairs, size_t count,
			struct mfkey32_result *results, size_t max_results);

#endif
