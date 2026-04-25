#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_SEND_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_SEND_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t *buf;
    size_t    cap;
    size_t    n;
    uint32_t  carrier_hz;
    uint8_t   duty_pct;
    bool      overflow;
} ir_send_buffer_t;

void ir_send_buffer_init(ir_send_buffer_t *b, uint16_t *buf, size_t cap,
                         uint32_t carrier_hz, uint8_t duty_pct);

void ir_send_mark(ir_send_buffer_t *b, uint16_t us);
void ir_send_space(ir_send_buffer_t *b, uint16_t us);

void ir_send_generic(ir_send_buffer_t *b,
                     uint16_t header_mark, uint16_t header_space,
                     uint16_t one_mark,    uint16_t one_space,
                     uint16_t zero_mark,   uint16_t zero_space,
                     uint16_t footer_mark, uint16_t footer_gap,
                     uint64_t data,        uint16_t nbits,
                     bool     msb_first);

#ifdef __cplusplus
}
#endif

#endif
