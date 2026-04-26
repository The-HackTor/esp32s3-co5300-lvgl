#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_DB_SEND_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_DB_SEND_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Per-protocol parametric sender for codec_db codecs. Builds a fresh raw
 * timing buffer from (address, command, raw_value) -- the same fields the
 * codec_db decoder produces. Round-trips bit-for-bit with the decoder.
 *
 * Caller frees *out_timings with free(). Returns ESP_OK on success or
 * ESP_ERR_NOT_SUPPORTED if no parametric sender is registered for that
 * protocol name (caller should fall back to RAW replay).
 */
typedef esp_err_t (*ir_codec_send_fn)(uint32_t address, uint32_t command,
                                      uint64_t raw_value,
                                      uint16_t **out_timings, size_t *out_n,
                                      uint32_t *out_freq_hz);

ir_codec_send_fn codec_db_send_lookup(const char *protocol_name);

esp_err_t ir_jvc_send  (uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq);
esp_err_t ir_sharp_send(uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq);
esp_err_t ir_denon_send(uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq);
esp_err_t ir_aiwa_send (uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq);
esp_err_t ir_nikai_send(uint32_t address, uint32_t command, uint64_t raw_value,
                        uint16_t **out_t, size_t *out_n, uint32_t *out_freq);

#ifdef __cplusplus
}
#endif

#endif
