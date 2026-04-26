#ifndef HW_SELFTEST_H
#define HW_SELFTEST_H

/* Aggressive 60-second boot-time hardware test:
 *   - Cycles RGB at full duty (R -> G -> B -> R...) every 200ms
 *   - TXes a 38 kHz IR burst every 200ms so a phone camera can see it
 *
 * Blocks app_main for the duration. Remove the single call site in
 * example_qspi_with_ram.c::app_main when no longer needed. */
void hw_selftest_run(void);

#endif
