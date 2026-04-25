#include "codec_match.h"

static uint32_t ticks_low(uint32_t desired_us, uint8_t tolerance_pct, uint16_t delta_us)
{
    uint32_t lower = desired_us - (desired_us * tolerance_pct) / 100U;
    if(lower > delta_us) lower -= delta_us;
    else lower = 0;
    return lower;
}

static uint32_t ticks_high(uint32_t desired_us, uint8_t tolerance_pct, uint16_t delta_us)
{
    return desired_us + (desired_us * tolerance_pct) / 100U + delta_us;
}

bool ir_match(uint32_t measured_us, uint32_t desired_us,
              uint8_t tolerance_pct, uint16_t delta_us)
{
    return measured_us >= ticks_low(desired_us, tolerance_pct, delta_us)
        && measured_us <= ticks_high(desired_us, tolerance_pct, delta_us);
}

bool ir_match_mark(uint16_t measured_us, uint32_t desired_us,
                   uint8_t tolerance_pct, int16_t excess_us)
{
    int32_t adjusted = (int32_t)desired_us + excess_us;
    if(adjusted < 0) adjusted = 0;
    return ir_match(measured_us, (uint32_t)adjusted, tolerance_pct, 0);
}

bool ir_match_space(uint16_t measured_us, uint32_t desired_us,
                    uint8_t tolerance_pct, int16_t excess_us)
{
    int32_t adjusted = (int32_t)desired_us - excess_us;
    if(adjusted < 0) adjusted = 0;
    return ir_match(measured_us, (uint32_t)adjusted, tolerance_pct, 0);
}

bool ir_match_at_least(uint16_t measured_us, uint32_t desired_us,
                       uint8_t tolerance_pct, uint16_t delta_us)
{
    if(measured_us == 0) return true;
    return (uint32_t)measured_us >= ticks_low(desired_us, tolerance_pct, delta_us);
}

uint64_t ir_reverse_bits(uint64_t input, uint16_t nbits)
{
    if(nbits == 0) return input;
    if(nbits > 64) nbits = 64;
    uint64_t output = 0;
    for(uint16_t i = 0; i < nbits; i++) {
        output = (output << 1) | (input & 1ULL);
        input >>= 1;
    }
    return output;
}

ir_match_result_t ir_match_data(
    const uint16_t *timings, size_t n_timings, size_t offset,
    uint16_t nbits,
    uint16_t one_mark, uint32_t one_space,
    uint16_t zero_mark, uint32_t zero_space,
    uint8_t  tolerance_pct, int16_t excess_us,
    bool     msb_first,
    bool     expect_last_space)
{
    ir_match_result_t result = { .success = false, .data = 0, .used = 0 };
    const size_t bits_with_space = expect_last_space ? nbits : (nbits ? nbits - 1 : 0);

    for(uint16_t bit = 0; bit < bits_with_space; bit++) {
        const size_t mi = offset + result.used;
        const size_t si = mi + 1;
        if(si >= n_timings) return result;

        if(ir_match_mark(timings[mi],  one_mark,  tolerance_pct, excess_us) &&
           ir_match_space(timings[si], one_space, tolerance_pct, excess_us)) {
            result.data = (result.data << 1) | 1ULL;
        } else if(ir_match_mark(timings[mi],  zero_mark,  tolerance_pct, excess_us) &&
                  ir_match_space(timings[si], zero_space, tolerance_pct, excess_us)) {
            result.data <<= 1;
        } else {
            if(!msb_first) result.data = ir_reverse_bits(result.data, result.used / 2);
            return result;
        }
        result.used += 2;
    }

    if(!expect_last_space && nbits > 0) {
        const size_t mi = offset + result.used;
        if(mi >= n_timings) return result;

        if(ir_match_mark(timings[mi], one_mark, tolerance_pct, excess_us)) {
            result.data = (result.data << 1) | 1ULL;
        } else if(ir_match_mark(timings[mi], zero_mark, tolerance_pct, excess_us)) {
            result.data <<= 1;
        } else {
            if(!msb_first) result.data = ir_reverse_bits(result.data, nbits ? nbits - 1 : 0);
            return result;
        }
        result.used += 1;
    }

    if(!msb_first) result.data = ir_reverse_bits(result.data, nbits);
    result.success = true;
    return result;
}

size_t ir_match_const_bit_time(const uint16_t *timings, size_t n_timings, size_t offset,
                               uint16_t nbits,
                               uint16_t header_mark, uint32_t header_space,
                               uint16_t one_pulse,   uint16_t zero_pulse,
                               uint16_t footer_mark, uint32_t footer_space,
                               bool     at_least_footer,
                               uint8_t  tolerance_pct, int16_t excess_us,
                               bool     msb_first,
                               uint64_t *out_value);

size_t ir_match_generic(const uint16_t *timings, size_t n_timings, size_t offset,
                        uint16_t nbits,
                        uint16_t header_mark, uint32_t header_space,
                        uint16_t one_mark,    uint32_t one_space,
                        uint16_t zero_mark,   uint32_t zero_space,
                        uint16_t footer_mark, uint32_t footer_space,
                        bool     at_least_footer,
                        uint8_t  tolerance_pct, int16_t excess_us,
                        bool     msb_first,
                        uint64_t *out_value)
{
    size_t cursor = offset;

    if(header_mark) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_mark(timings[cursor], header_mark, tolerance_pct, excess_us)) return 0;
        cursor++;
    }
    if(header_space) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_space(timings[cursor], header_space, tolerance_pct, excess_us)) return 0;
        cursor++;
    }

    const bool expect_last_space = (footer_mark != 0);
    ir_match_result_t bits = ir_match_data(
        timings, n_timings, cursor, nbits,
        one_mark, one_space, zero_mark, zero_space,
        tolerance_pct, excess_us, msb_first, expect_last_space);
    if(!bits.success) return 0;
    cursor += bits.used;

    if(footer_mark) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_mark(timings[cursor], footer_mark, tolerance_pct, excess_us)) return 0;
        cursor++;
    }
    if(footer_space) {
        const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
        if(at_least_footer) {
            if(!ir_match_at_least(measured, footer_space, tolerance_pct, 0)) return 0;
        } else {
            if(!ir_match_space(measured, footer_space, tolerance_pct, excess_us)) return 0;
        }
        if(cursor < n_timings) cursor++;
    }

    if(out_value) *out_value = bits.data;
    return cursor - offset;
}

size_t ir_match_const_bit_time(const uint16_t *timings, size_t n_timings, size_t offset,
                               uint16_t nbits,
                               uint16_t header_mark, uint32_t header_space,
                               uint16_t one_pulse,   uint16_t zero_pulse,
                               uint16_t footer_mark, uint32_t footer_space,
                               bool     at_least_footer,
                               uint8_t  tolerance_pct, int16_t excess_us,
                               bool     msb_first,
                               uint64_t *out_value)
{
    if(footer_mark) {
        return ir_match_generic(timings, n_timings, offset, nbits,
                                header_mark, header_space,
                                one_pulse,  zero_pulse,
                                zero_pulse, one_pulse,
                                footer_mark, footer_space,
                                at_least_footer,
                                tolerance_pct, excess_us, msb_first, out_value);
    }

    const uint16_t bits_with_space = nbits ? (uint16_t)(nbits - 1) : 0;
    uint64_t result = 0;
    size_t consumed = ir_match_generic(timings, n_timings, offset, bits_with_space,
                                       header_mark, header_space,
                                       one_pulse,  zero_pulse,
                                       zero_pulse, one_pulse,
                                       0, 0, false,
                                       tolerance_pct, excess_us, true, &result);
    if(consumed == 0) return 0;

    size_t cursor = offset + consumed;
    if(cursor >= n_timings) return 0;

    bool last_bit;
    if(ir_match_mark(timings[cursor], one_pulse, tolerance_pct, excess_us)) {
        last_bit = true;
        result = (result << 1) | 1ULL;
    } else if(ir_match_mark(timings[cursor], zero_pulse, tolerance_pct, excess_us)) {
        last_bit = false;
        result <<= 1;
    } else {
        return 0;
    }
    cursor++;

    const uint32_t expected_space = (uint32_t)(last_bit ? zero_pulse : one_pulse) + footer_space;
    if(cursor < n_timings) {
        if(at_least_footer) {
            if(!ir_match_at_least(timings[cursor], expected_space, tolerance_pct, 0)) return 0;
        } else {
            if(!ir_match_space(timings[cursor], expected_space, tolerance_pct, excess_us)) return 0;
        }
        cursor++;
    }

    if(!msb_first) result = ir_reverse_bits(result, nbits);
    if(out_value) *out_value = result;
    return cursor - offset;
}

size_t ir_match_manchester(const uint16_t *timings, size_t n_timings, size_t offset,
                           uint16_t nbits,
                           uint16_t header_mark, uint32_t header_space,
                           uint16_t half_period_us,
                           uint16_t footer_mark, uint32_t footer_space,
                           bool     at_least_footer,
                           uint8_t  tolerance_pct, int16_t excess_us,
                           bool     ge_thomas,
                           bool     msb_first,
                           uint64_t *out_value)
{
    size_t cursor = offset;

    if(header_mark) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_mark(timings[cursor], header_mark, tolerance_pct, excess_us)) return 0;
        cursor++;
    }
    uint32_t starting_balance = 0;
    if(header_space) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_at_least(timings[cursor], header_space, tolerance_pct, 0)) return 0;
        if(timings[cursor] > header_space) starting_balance = timings[cursor] - header_space;
        cursor++;
    }

    uint64_t data = 0;
    bool current_bit = starting_balance ? !ge_thomas : ge_thomas;
    uint16_t nr_half = 0;
    const uint16_t expected_half = (uint16_t)(nbits * 2);
    uint32_t bank = starting_balance;

    while((cursor < n_timings || bank) && nr_half < expected_half) {
        if(!bank) {
            if(cursor >= n_timings) return 0;
            bank = timings[cursor++];
        }
        if(!ir_match(bank, half_period_us, tolerance_pct, 0)) return 0;
        nr_half++;

        if(cursor < n_timings) {
            bank = timings[cursor++];
        } else if(cursor == n_timings) {
            bank = half_period_us;
        } else {
            return 0;
        }

        data = (data << 1) | (current_bit ? 1ULL : 0ULL);

        if(ir_match(bank, (uint32_t)half_period_us * 2U, tolerance_pct, 0)) {
            current_bit = !current_bit;
            bank = (bank > half_period_us) ? bank - half_period_us : 0;
        } else if(ir_match(bank, half_period_us, tolerance_pct, 0)) {
            bank = 0;
        } else if(nr_half == expected_half - 1 &&
                  ir_match_at_least((uint16_t)bank, half_period_us, tolerance_pct, 0)) {
            bank = 0;
            if(cursor > offset) cursor--;
        } else {
            return 0;
        }
        nr_half++;
    }

    if(footer_mark) {
        if(cursor >= n_timings) return 0;
        if(!ir_match_mark(timings[cursor], footer_mark, tolerance_pct, excess_us)) return 0;
        cursor++;
    }
    if(footer_space) {
        const uint16_t measured = (cursor < n_timings) ? timings[cursor] : 0;
        if(at_least_footer) {
            if(!ir_match_at_least(measured, footer_space, tolerance_pct, 0)) return 0;
        } else {
            if(!ir_match_space(measured, footer_space, tolerance_pct, excess_us)) return 0;
        }
        if(cursor < n_timings) cursor++;
    }

    if(!msb_first) data = ir_reverse_bits(data, nbits);
    if(out_value) *out_value = data;
    return cursor - offset;
}

size_t ir_match_data_ratio(const uint16_t *timings, size_t n_timings, size_t offset,
                           uint16_t nbits,
                           uint8_t  one_pct_min, uint8_t one_pct_max,
                           uint8_t  zero_pct_min, uint8_t zero_pct_max,
                           bool     msb_first,
                           uint64_t *out_value)
{
    uint64_t data = 0;
    size_t cursor = offset;
    for(uint16_t i = 0; i < nbits; i++) {
        if(cursor + 1 >= n_timings) return 0;
        const uint32_t mark  = timings[cursor];
        const uint32_t space = timings[cursor + 1];
        const uint32_t total = mark + space;
        if(total == 0) return 0;
        const uint8_t pct = (uint8_t)((mark * 100U) / total);
        if(pct >= one_pct_min && pct <= one_pct_max) {
            data = (data << 1) | 1ULL;
        } else if(pct >= zero_pct_min && pct <= zero_pct_max) {
            data <<= 1;
        } else {
            return 0;
        }
        cursor += 2;
    }
    if(!msb_first) data = ir_reverse_bits(data, nbits);
    if(out_value) *out_value = data;
    return cursor - offset;
}

size_t ir_match_data_rle(const uint16_t *timings, size_t n_timings, size_t offset,
                         uint16_t nbits,
                         uint16_t base_period_us,
                         uint8_t  tolerance_pct,
                         bool     starts_with_mark,
                         bool     msb_first,
                         uint64_t *out_value)
{
    if(base_period_us == 0) return 0;
    const uint16_t tol_us = (uint16_t)((uint32_t)base_period_us * tolerance_pct / 100U);
    const uint16_t half = base_period_us / 2 + tol_us;

    uint64_t data = 0;
    uint16_t emitted = 0;
    size_t cursor = offset;
    bool current_polarity = starts_with_mark;

    while(emitted < nbits) {
        if(cursor >= n_timings) return 0;
        const uint32_t dur = timings[cursor];
        if(dur == 0) return 0;
        const uint32_t k = (dur + half) / base_period_us;
        if(k == 0) return 0;
        for(uint32_t i = 0; i < k && emitted < nbits; i++) {
            data = (data << 1) | (current_polarity ? 1ULL : 0ULL);
            emitted++;
        }
        current_polarity = !current_polarity;
        cursor++;
    }

    if(!msb_first) data = ir_reverse_bits(data, nbits);
    if(out_value) *out_value = data;
    return cursor - offset;
}
