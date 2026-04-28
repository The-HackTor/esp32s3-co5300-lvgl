#ifndef SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_
#define SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

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

/*
 * Encode a decoded message back to a raw mark/space timing buffer suitable
 * for hw_ir_send_raw. Caller frees *out_timings with free(). Returns
 * ESP_OK on success, ESP_ERR_NOT_SUPPORTED if the protocol name doesn't
 * match a Flipper-tier encoder (codec_db / IRremoteESP8266 protocols are
 * decode-only for now).
 */
esp_err_t ir_codecs_encode(const IrDecoded *in,
                           uint16_t **out_timings, size_t *out_n,
                           uint32_t *out_freq_hz);

/*
 * Like ir_codecs_encode but also captures one cycle of the protocol's
 * repeat-sequence following the first frame. NEC/NECext/NEC42/Samsung32
 * have a short repeat code (the 9000/2250/562 sequence and similar);
 * other Flipper-tier protocols re-fire the full frame on hold. Either
 * way, *out_repeat_t is what should be sent for every hold-tick AFTER
 * the first frame so a held button looks to the receiver like one
 * sustained press, not a burst of distinct presses.
 *
 * On success, callers must free both *out_timings and *out_repeat_t.
 * If *out_repeat_n is 0 (codec_db / IRremoteESP8266 protocols without
 * a Flipper-tier encoder), the repeat buffer is unset; the hold-tick
 * caller should re-fire the full frame.
 */
esp_err_t ir_codecs_encode_with_repeat(const IrDecoded *in,
                                       uint16_t **out_timings,        size_t *out_n,
                                       uint16_t **out_repeat_timings, size_t *out_repeat_n,
                                       uint32_t *out_freq_hz);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* SMART_FLIPPER_LIB_INFRARED_IR_CODECS_H_ */
