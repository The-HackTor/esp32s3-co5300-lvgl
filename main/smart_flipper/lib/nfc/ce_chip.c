#include "ce_chip.h"
#include "nfc_trace.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"

#include <errno.h>
#include <string.h>

#define CE_LISTENER_IRQS ( \
    ST25R3916_IRQ_MASK_FWL   | ST25R3916_IRQ_MASK_TXE    | \
    ST25R3916_IRQ_MASK_RXS   | ST25R3916_IRQ_MASK_RXE    | \
    ST25R3916_IRQ_MASK_PAR   | ST25R3916_IRQ_MASK_CRC    | \
    ST25R3916_IRQ_MASK_ERR1  | ST25R3916_IRQ_MASK_ERR2   | \
    ST25R3916_IRQ_MASK_NRE   | ST25R3916_IRQ_MASK_EON    | \
    ST25R3916_IRQ_MASK_EOF   | ST25R3916_IRQ_MASK_WU_A_X | \
    ST25R3916_IRQ_MASK_WU_A  | ST25R3916_IRQ_MASK_RXE_PTA)

static void ce_chip_rx_config(void)
{
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF1,
                           ST25R3916_REG_RX_CONF1_z600k);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF2,
                           ST25R3916_REG_RX_CONF2_agc6_3 |
                           ST25R3916_REG_RX_CONF2_agc_m |
                           ST25R3916_REG_RX_CONF2_agc_en |
                           ST25R3916_REG_RX_CONF2_sqm_dyn);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF3, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF4, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_CORR_CONF1,
                           ST25R3916_REG_CORR_CONF1_corr_s0 |
                           ST25R3916_REG_CORR_CONF1_corr_s4 |
                           ST25R3916_REG_CORR_CONF1_corr_s6);
    st25r3916WriteRegister(ST25R3916_REG_CORR_CONF2, 0x00);
}

static void ce_chip_write_pt(const struct iso14443a_card *card)
{
    if(card->uid_len == 4) {
        st25r3916ChangeRegisterBits(ST25R3916_REG_AUX,
                                    ST25R3916_REG_AUX_nfc_id_mask,
                                    ST25R3916_REG_AUX_nfc_id_4bytes);
    } else {
        st25r3916ChangeRegisterBits(ST25R3916_REG_AUX,
                                    ST25R3916_REG_AUX_nfc_id_mask,
                                    ST25R3916_REG_AUX_nfc_id_7bytes);
    }

    uint8_t pt[15];
    memset(pt, 0, sizeof(pt));
    memcpy(pt, card->uid, card->uid_len);
    pt[10] = card->atqa[0];
    pt[11] = card->atqa[1];

    uint8_t sak_final = card->sak & ~0x04;
    if(card->uid_len == 4) {
        pt[12] = sak_final;
    } else {
        pt[12] = 0x04;
    }
    pt[13] = sak_final;
    pt[14] = sak_final;

    st25r3916WritePTMem(pt, sizeof(pt));
}

void ce_chip_listener_init(const struct iso14443a_card *card)
{
    st25r3916WriteRegister(ST25R3916_REG_OP_CONTROL,
                           ST25R3916_REG_OP_CONTROL_en |
                           ST25R3916_REG_OP_CONTROL_rx_en |
                           ST25R3916_REG_OP_CONTROL_en_fd_auto_efd);

    st25r3916WriteRegister(ST25R3916_REG_MODE,
                           ST25R3916_REG_MODE_targ_targ |
                           ST25R3916_REG_MODE_om0);

    st25r3916WriteRegister(ST25R3916_REG_PASSIVE_TARGET,
                           ST25R3916_REG_PASSIVE_TARGET_fdel_2 |
                           ST25R3916_REG_PASSIVE_TARGET_fdel_0 |
                           ST25R3916_REG_PASSIVE_TARGET_d_ac_ap2p |
                           ST25R3916_REG_PASSIVE_TARGET_d_212_424_1r);

    st25r3916WriteRegister(ST25R3916_REG_MASK_RX_TIMER, 0x02);

    st25r3916ExecuteCommand(ST25R3916_CMD_STOP);

    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_ALL);
    st25r3916EnableInterrupts(CE_LISTENER_IRQS);

    ce_chip_write_pt(card);

    st25r3916ClrRegisterBits(ST25R3916_REG_ISO14443A_NFC,
                             ST25R3916_REG_ISO14443A_NFC_no_tx_par);

    st25r3916ClrRegisterBits(ST25R3916_REG_PASSIVE_TARGET,
                             ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a);

    st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE);

    ce_chip_rx_config();
}

void ce_chip_listener_sleep(void)
{
    st25r3916ClrRegisterBits(ST25R3916_REG_PASSIVE_TARGET,
                             ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a);
    st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SLEEP);
}

void ce_chip_listener_idle(void)
{
    st25r3916ClrRegisterBits(ST25R3916_REG_PASSIVE_TARGET,
                             ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a);
    st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
    st25r3916ExecuteCommand(ST25R3916_CMD_GOTO_SENSE);
}

void ce_chip_prepare_rx(void)
{
    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_ALL);
    st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916ExecuteCommand(ST25R3916_CMD_UNMASK_RECEIVE_DATA);
}

void ce_chip_activate_pta(void)
{
    st25r3916SetRegisterBits(ST25R3916_REG_PASSIVE_TARGET,
                             ST25R3916_REG_PASSIVE_TARGET_d_106_ac_a);
    st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916ExecuteCommand(ST25R3916_CMD_UNMASK_RECEIVE_DATA);
}

void ce_chip_fifo_tx(const uint8_t *data, uint16_t bits)
{
    uint16_t bytes = (bits + 7) / 8;
    st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_TXE);
    st25r3916SetNumTxBits(bits);
    st25r3916WriteFifo(data, bytes);
    nfc_trace_append(NFC_TRACE_DIR_C2R, data, bytes);
    st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITHOUT_CRC);
    st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 50);
}

void ce_chip_fifo_tx_with_crc(const uint8_t *data, uint16_t bytes)
{
    st25r3916ExecuteCommand(ST25R3916_CMD_CLEAR_FIFO);
    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_TXE);
    st25r3916SetNumTxBits(bytes * 8);
    st25r3916WriteFifo(data, bytes);
    nfc_trace_append(NFC_TRACE_DIR_C2R, data, bytes);
    st25r3916ExecuteCommand(ST25R3916_CMD_TRANSMIT_WITH_CRC);
    st25r3916WaitForInterruptsTimed(ST25R3916_IRQ_MASK_TXE, 50);
}

int ce_chip_fifo_rx(uint8_t *data, uint16_t max_len, uint16_t *rx_len,
                    uint16_t timeout_ms)
{
    uint32_t irqs = st25r3916WaitForInterruptsTimed(
        ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA |
        ST25R3916_IRQ_MASK_EOF | ST25R3916_IRQ_MASK_ERR1,
        timeout_ms);

    if(irqs & ST25R3916_IRQ_MASK_EOF) return -ENODEV;
    if((irqs & (ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_RXE_PTA)) == 0)
        return -ETIMEDOUT;

    uint16_t fifo_bytes = st25r3916GetNumFIFOBytes();
    if(fifo_bytes > max_len) fifo_bytes = max_len;
    st25r3916ReadFifo(data, fifo_bytes);
    *rx_len = fifo_bytes;

    if(*rx_len > 0) {
        nfc_trace_append(NFC_TRACE_DIR_R2C, data, *rx_len);
    }

    st25r3916GetInterrupt(ST25R3916_IRQ_MASK_CRC | ST25R3916_IRQ_MASK_PAR |
                          ST25R3916_IRQ_MASK_ERR1 | ST25R3916_IRQ_MASK_ERR2);
    return 0;
}
