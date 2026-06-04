#ifndef CE_CHIP_H
#define CE_CHIP_H

#include <nfc.h>
#include <stdint.h>

void ce_chip_listener_init(const struct iso14443a_card *card);
void ce_chip_listener_sleep(void);
void ce_chip_listener_idle(void);
void ce_chip_prepare_rx(void);
void ce_chip_activate_pta(void);

void ce_chip_fifo_tx(const uint8_t *data, uint16_t bits);
void ce_chip_fifo_tx_with_crc(const uint8_t *data, uint16_t bytes);

int  ce_chip_fifo_rx(uint8_t *data, uint16_t max_len, uint16_t *rx_len,
                     uint16_t timeout_ms);

#endif
