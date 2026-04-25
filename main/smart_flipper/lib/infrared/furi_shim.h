/*
 * Furi → ESP-IDF compatibility shim for the lifted Flipper IR codec tree.
 *
 * The Flipper codec sources under encoder_decoder/ depend on three Furi
 * headers (<core/check.h>, <core/common_defines.h>, <core/core_defines.h>)
 * for asserts, panics, and a few utility macros. This single shim replaces
 * all three -- the lift step rewrites those includes to point here, and the
 * codec sources are otherwise unmodified for round-trip portability with
 * upstream Flipper firmware.
 */

#ifndef SMART_FLIPPER_LIB_INFRARED_FURI_SHIM_H_
#define SMART_FLIPPER_LIB_INFRARED_FURI_SHIM_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>   /* abort() */

/* ---- furi_assert / furi_check / furi_crash ---- */
/* Flipper's furi_assert is a debug-only check; furi_check is always-on; both
 * panic the system on failure. Under ESP-IDF we route both to the standard
 * `assert` -- with NDEBUG off (default IDF), assert traps to the panic
 * handler with a meaningful message. */
#define furi_assert(...)  assert(_FURI_FIRST(__VA_ARGS__))
#define furi_check(...)   assert(_FURI_FIRST(__VA_ARGS__))
#define furi_crash(...)   abort()

/* Helper to extract the first VA_ARG (so furi_assert(cond, "msg") still
 * compiles by dropping the message). */
#define _FURI_FIRST(...)        _FURI_FIRST_HELPER(__VA_ARGS__, 0)
#define _FURI_FIRST_HELPER(a, ...) (a)

/* ---- core_defines.h utility macros used by codecs ---- */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef COUNT_OF
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))
#endif

#endif  /* SMART_FLIPPER_LIB_INFRARED_FURI_SHIM_H_ */
