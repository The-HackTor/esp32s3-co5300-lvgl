#ifndef SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_MATCH_H_
#define SMART_FLIPPER_LIB_INFRARED_CODEC_DB_CODEC_MATCH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IR_DEFAULT_TOLERANCE 25
#define IR_MARK_EXCESS       50

bool ir_match(uint32_t measured_us, uint32_t desired_us,
              uint8_t tolerance_pct, uint16_t delta_us);

bool ir_match_mark(uint16_t measured_us, uint32_t desired_us,
                   uint8_t tolerance_pct, int16_t excess_us);

bool ir_match_space(uint16_t measured_us, uint32_t desired_us,
                    uint8_t tolerance_pct, int16_t excess_us);

bool ir_match_at_least(uint16_t measured_us, uint32_t desired_us,
                       uint8_t tolerance_pct, uint16_t delta_us);

typedef struct {
    bool     success;
    uint64_t data;
    size_t   used;
} ir_match_result_t;

ir_match_result_t ir_match_data(
    const uint16_t *timings, size_t n_timings, size_t offset,
    uint16_t nbits,
    uint16_t one_mark, uint32_t one_space,
    uint16_t zero_mark, uint32_t zero_space,
    uint8_t  tolerance_pct, int16_t excess_us,
    bool     msb_first,
    bool     expect_last_space);

size_t ir_match_generic(const uint16_t *timings, size_t n_timings, size_t offset,
                        uint16_t nbits,
                        uint16_t header_mark, uint32_t header_space,
                        uint16_t one_mark,    uint32_t one_space,
                        uint16_t zero_mark,   uint32_t zero_space,
                        uint16_t footer_mark, uint32_t footer_space,
                        bool     at_least_footer,
                        uint8_t  tolerance_pct, int16_t excess_us,
                        bool     msb_first,
                        uint64_t *out_value);

uint64_t ir_reverse_bits(uint64_t input, uint16_t nbits);

size_t ir_match_const_bit_time(const uint16_t *timings, size_t n_timings, size_t offset,
                               uint16_t nbits,
                               uint16_t header_mark, uint32_t header_space,
                               uint16_t one_pulse,   uint16_t zero_pulse,
                               uint16_t footer_mark, uint32_t footer_space,
                               bool     at_least_footer,
                               uint8_t  tolerance_pct, int16_t excess_us,
                               bool     msb_first,
                               uint64_t *out_value);

size_t ir_match_manchester(const uint16_t *timings, size_t n_timings, size_t offset,
                           uint16_t nbits,
                           uint16_t header_mark, uint32_t header_space,
                           uint16_t half_period_us,
                           uint16_t footer_mark, uint32_t footer_space,
                           bool     at_least_footer,
                           uint8_t  tolerance_pct, int16_t excess_us,
                           bool     ge_thomas,
                           bool     msb_first,
                           uint64_t *out_value);

size_t ir_match_data_ratio(const uint16_t *timings, size_t n_timings, size_t offset,
                           uint16_t nbits,
                           uint8_t  one_pct_min, uint8_t one_pct_max,
                           uint8_t  zero_pct_min, uint8_t zero_pct_max,
                           bool     msb_first,
                           uint64_t *out_value);

size_t ir_match_data_rle(const uint16_t *timings, size_t n_timings, size_t offset,
                         uint16_t nbits,
                         uint16_t base_period_us,
                         uint8_t  tolerance_pct,
                         bool     starts_with_mark,
                         bool     msb_first,
                         uint64_t *out_value);

#ifdef __cplusplus
}
#endif

#endif
