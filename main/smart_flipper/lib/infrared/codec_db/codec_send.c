#include "codec_send.h"

#include <string.h>

void ir_send_buffer_init(ir_send_buffer_t *b, uint16_t *buf, size_t cap,
                         uint32_t carrier_hz, uint8_t duty_pct)
{
    b->buf        = buf;
    b->cap        = cap;
    b->n          = 0;
    b->carrier_hz = carrier_hz;
    b->duty_pct   = duty_pct;
    b->overflow   = false;
    if(buf && cap) memset(buf, 0, cap * sizeof(buf[0]));
}

static void emit(ir_send_buffer_t *b, uint16_t us)
{
    if(b->n >= b->cap) { b->overflow = true; return; }
    b->buf[b->n++] = us;
}

void ir_send_mark(ir_send_buffer_t *b, uint16_t us)  { emit(b, us); }
void ir_send_space(ir_send_buffer_t *b, uint16_t us) { emit(b, us); }

void ir_send_generic(ir_send_buffer_t *b,
                     uint16_t header_mark, uint16_t header_space,
                     uint16_t one_mark,    uint16_t one_space,
                     uint16_t zero_mark,   uint16_t zero_space,
                     uint16_t footer_mark, uint16_t footer_gap,
                     uint64_t data,        uint16_t nbits,
                     bool     msb_first)
{
    if(header_mark)  ir_send_mark(b, header_mark);
    if(header_space) ir_send_space(b, header_space);

    for(uint16_t i = 0; i < nbits; i++) {
        const uint16_t bit_idx = msb_first ? (uint16_t)(nbits - 1 - i) : i;
        const bool one = (data >> bit_idx) & 1ULL;
        if(one) {
            ir_send_mark(b,  one_mark);
            ir_send_space(b, one_space);
        } else {
            ir_send_mark(b,  zero_mark);
            ir_send_space(b, zero_space);
        }
    }

    if(footer_mark) ir_send_mark(b, footer_mark);
    if(footer_gap)  ir_send_space(b, footer_gap);
}
