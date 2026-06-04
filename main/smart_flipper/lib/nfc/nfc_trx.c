#include "nfc_trx.h"
#include "nfc_trace.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"
#include "esp_log.h"
#include <errno.h>

static const char *TAG = "nfc_trx";

#define TRX_IRQ_MASK (ST25R3916_IRQ_MASK_FWL  | ST25R3916_IRQ_MASK_TXE  | \
                      ST25R3916_IRQ_MASK_RXS  | ST25R3916_IRQ_MASK_RXE  | \
                      ST25R3916_IRQ_MASK_PAR  | ST25R3916_IRQ_MASK_CRC  | \
                      ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2 | \
                      ST25R3916_IRQ_MASK_NRE  | ST25R3916_IRQ_MASK_COL  | \
                      ST25R3916_IRQ_MASK_RXE_PTA)

static void trx_prepare(void)
{
    st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916ClrRegisterBits(ST25R3916_REG_TIMER_EMV_CONTROL,
                             ST25R3916_REG_TIMER_EMV_CONTROL_nrt_emv);
    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_ALL);
    st25r3916EnableInterrupts(TRX_IRQ_MASK);
}

static int trx_wait_rx(uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
                       uint16_t timeout_ms)
{
    uint32_t irqs = st25r3916WaitForInterruptsTimed(
        ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA |
        ST25R3916_IRQ_MASK_NRE |
        ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2,
        timeout_ms);

    if(irqs & ST25R3916_IRQ_MASK_NRE) { *rx_len = 0; return -ETIMEDOUT; }
    if(irqs & (ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2)) {
        *rx_len = 0; return -EIO;
    }
    if(!(irqs & (ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA))) {
        *rx_len = 0; return -ETIMEDOUT;
    }

    uint16_t fifo_bytes = st25r3916GetNumFIFOBytes();
    if(fifo_bytes > rx_size) fifo_bytes = rx_size;
    if(fifo_bytes > 0) st25r3916ReadFifo(rx, fifo_bytes);
    *rx_len = fifo_bytes;
    return 0;
}

int nfc_trx(const uint8_t *tx, uint16_t tx_bytes,
            uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
            uint16_t timeout_ms, bool tx_crc, bool rx_crc)
{
    *rx_len = 0;
    trx_prepare();

    st25r3916ClrRegisterBits(ST25R3916_REG_ISO14443A_NFC,
                             ST25R3916_REG_ISO14443A_NFC_no_tx_par |
                             ST25R3916_REG_ISO14443A_NFC_no_rx_par);

    if(!rx_crc) st25r3916SetRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);
    else        st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);

    st25r3916SetNumTxBits(tx_bytes * 8);
    st25r3916WriteFifo(tx, tx_bytes);

    if(tx && tx_bytes) nfc_trace_append(NFC_TRACE_DIR_R2C, tx, tx_bytes);

    if(tx_crc) st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    else       st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITHOUT_CRC);

    st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 50);

    int ret = trx_wait_rx(rx, rx_size, rx_len, timeout_ms);
    if(ret == 0 && *rx_len > 0) {
        nfc_trace_append(NFC_TRACE_DIR_C2R, rx, *rx_len);
    }
    (void)TAG;
    return ret;
}

int nfc_trx_custom_parity(const uint8_t *tx, uint16_t tx_bits,
                          uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
                          uint16_t timeout_ms)
{
    *rx_len = 0;
    trx_prepare();

    st25r3916ModifyRegister(ST25R3916_REG_ISO14443A_NFC,
                            ST25R3916_REG_ISO14443A_NFC_no_tx_par |
                            ST25R3916_REG_ISO14443A_NFC_no_rx_par,
                            ST25R3916_REG_ISO14443A_NFC_no_tx_par);

    st25r3916SetRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);

    uint16_t tx_bytes = (tx_bits + 7) / 8;
    st25r3916SetNumTxBits(tx_bits);
    st25r3916WriteFifo(tx, tx_bytes);

    if(tx && tx_bytes) nfc_trace_append(NFC_TRACE_DIR_R2C, tx, tx_bytes);

    st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITHOUT_CRC);
    st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 50);

    int ret = trx_wait_rx(rx, rx_size, rx_len, timeout_ms);
    if(ret == 0 && *rx_len > 0) nfc_trace_append(NFC_TRACE_DIR_C2R, rx, *rx_len);

    st25r3916ClrRegisterBits(ST25R3916_REG_ISO14443A_NFC,
                             ST25R3916_REG_ISO14443A_NFC_no_tx_par);
    st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);
    return ret;
}

int nfc_trx_short_frame(uint8_t cmd,
                        uint8_t *rx, uint16_t rx_size, uint16_t *rx_len,
                        uint16_t timeout_ms)
{
    *rx_len = 0;
    trx_prepare();

    st25r3916ClrRegisterBits(ST25R3916_REG_ISO14443A_NFC,
                             ST25R3916_REG_ISO14443A_NFC_no_tx_par |
                             ST25R3916_REG_ISO14443A_NFC_no_rx_par);
    st25r3916SetRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);

    nfc_trace_append(NFC_TRACE_DIR_R2C, &cmd, 1);

    st25r3916ExecuteCommand(cmd);
    st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 50);

    uint32_t rxirqs = st25r3916WaitForInterruptsTimed(
        ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA |
        ST25R3916_IRQ_MASK_NRE |
        ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2,
        timeout_ms);

    if(rxirqs & ST25R3916_IRQ_MASK_NRE) {
        *rx_len = 0;
        st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);
        return -ETIMEDOUT;
    }
    if(!(rxirqs & (ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA))) {
        *rx_len = 0;
        st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);
        return -ETIMEDOUT;
    }

    uint16_t fifo_bytes = st25r3916GetNumFIFOBytes();
    if(fifo_bytes > rx_size) fifo_bytes = rx_size;
    if(fifo_bytes > 0) st25r3916ReadFifo(rx, fifo_bytes);
    *rx_len = fifo_bytes;

    if(*rx_len > 0) nfc_trace_append(NFC_TRACE_DIR_C2R, rx, *rx_len);

    st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_no_crc_rx);
    return 0;
}
