#ifndef SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_
#define SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * C-callable two-tier IR codec dispatch.
 *
 *   tier 1: Flipper firmware codecs (NEC/NECext/NEC42, RC5/RC5X, RC6,
 *           Samsung32, SIRC/SIRC15/SIRC20, Kaseikyo, Pioneer, RCA).
 *           Decoded fields map 1:1 to Flipper's .ir text format, so
 *           saved files round-trip to a real Flipper.
 *   tier 2: IRremoteESP8266 (slice 7b) -- 80+ protocols including AC
 *           remotes that Flipper doesn't recognize.
 *
 * Input is a flat alternating mark/space buffer (microseconds). hw_ir.c
 * RX produces this format directly.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IR_DECODED_NONE = 0,
    IR_DECODED_FLIPPER,        /* protocol/address/command map to Flipper enum */
    IR_DECODED_IRREMOTEESP,    /* protocol = upstream name string */
} IrDecodedSource;

typedef struct {
    IrDecodedSource source;
    char            protocol[24];   /* text name, Flipper-format-compatible */
    uint32_t        address;
    uint32_t        command;
    uint64_t        raw_value;      /* IRremoteESP8266 only -- AC frame bytes etc. */
    bool            repeat;
} IrDecoded;

/*
 * Run all available codecs against `timings` (alternating mark/space, us).
 * Tier order: Flipper first, IRremoteESP8266 fallback. Returns true and
 * fills *out on the first match. *out is zeroed on entry.
 */
bool ir_codecs_decode(const uint16_t *timings, size_t n_timings, IrDecoded *out);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_ */
