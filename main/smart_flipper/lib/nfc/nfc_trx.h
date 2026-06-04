#ifndef NFC_TRX_H
#define NFC_TRX_H

#include <stdint.h>
#include <stdbool.h>
#include "st25r3916_irq.h"

#define NFC_TRX_IRQ_MASK  (ST25R3916_IRQ_MASK_FWL  | ST25R3916_IRQ_MASK_TXE  | \
                           ST25R3916_IRQ_MASK_RXS  | ST25R3916_IRQ_MASK_RXE  | \
                           ST25R3916_IRQ_MASK_PAR  | ST25R3916_IRQ_MASK_CRC  | \
                           ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2 | \
                           ST25R3916_IRQ_MASK_NRE)

int nfc_trx(const uint8_t *tx, uint16_t tx_bytes,
            uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
            uint16_t timeout_ms, bool tx_crc, bool rx_crc);

int nfc_trx_custom_parity(const uint8_t *tx, uint16_t tx_bits,
                           uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
                           uint16_t timeout_ms);

int nfc_trx_short_frame(uint8_t cmd,
                        uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
                        uint16_t timeout_ms);

#endif
