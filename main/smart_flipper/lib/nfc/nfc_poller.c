#include "nfc_poller.h"
#include "nfc_trx.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916_irq.h"
#include "st25r3916_aat.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <errno.h>
#include <string.h>

static const char *TAG = "nfc_poller";

#define ISO14443A_SEL_CMD_CL1  0x93
#define ISO14443A_SEL_CMD_CL2  0x95
#define ISO14443A_SEL_CMD_CL3  0x97
#define ISO14443A_SEL_PAR_NVB7 0x70
#define ISO14443A_SEL_PAR_NVB2 0x20
#define ISO14443A_CT           0x88
#define ISO14443A_HLTA_CMD1    0x50
#define ISO14443A_HLTA_CMD2    0x00

#define REQA_TIMEOUT_MS        20
#define SELECT_TIMEOUT_MS      50

static inline int64_t uptime_ms(void) { return esp_timer_get_time() / 1000; }
static inline void    msleep(uint32_t n) { vTaskDelay(pdMS_TO_TICKS(n)); }

void nfc_chip_config(void)
{
    st25r3916ModifyRegister(ST25R3916_REG_IO_CONF1,
                            ST25R3916_REG_IO_CONF1_out_cl_mask |
                            ST25R3916_REG_IO_CONF1_lf_clk_off,
                            0x07);

    st25r3916ClrRegisterBits(ST25R3916_REG_IO_CONF2,
                             ST25R3916_REG_IO_CONF2_miso_pd1 |
                             ST25R3916_REG_IO_CONF2_miso_pd2);

    st25r3916SetRegisterBits(ST25R3916_REG_IO_CONF2,
                             ST25R3916_REG_IO_CONF2_aat_en);

    st25r3916ModifyRegister(ST25R3916_REG_TX_DRIVER,
                            ST25R3916_REG_TX_DRIVER_d_res_mask, 0x00);

    st25r3916WriteRegister(ST25R3916_REG_RES_AM_MOD, 0x80);

    st25r3916ModifyRegister(ST25R3916_REG_FIELD_THRESHOLD_ACTV, 0xFF,
                            ST25R3916_REG_FIELD_THRESHOLD_ACTV_trg_105mV |
                            ST25R3916_REG_FIELD_THRESHOLD_ACTV_rfe_105mV);

    st25r3916ModifyRegister(ST25R3916_REG_FIELD_THRESHOLD_DEACTV, 0xFF,
                            ST25R3916_REG_FIELD_THRESHOLD_DEACTV_trg_75mV |
                            ST25R3916_REG_FIELD_THRESHOLD_DEACTV_rfe_150mV);

    st25r3916SetRegisterBits(ST25R3916_REG_AUX_MOD,
                             ST25R3916_REG_AUX_MOD_lm_ext |
                             ST25R3916_REG_AUX_MOD_lm_dri);

    st25r3916ModifyRegister(ST25R3916_REG_PASSIVE_TARGET,
                            ST25R3916_REG_PASSIVE_TARGET_fdel_mask,
                            5U << ST25R3916_REG_PASSIVE_TARGET_fdel_shift);

    st25r3916WriteRegister(ST25R3916_REG_PT_MOD, 0x0F);

    st25r3916SetRegisterBits(ST25R3916_REG_EMD_SUP_CONF,
                             ST25R3916_REG_EMD_SUP_CONF_rx_start_emv);

    st25r3916WriteRegister(ST25R3916_REG_ANT_TUNE_A, 0x82);
    st25r3916WriteRegister(ST25R3916_REG_ANT_TUNE_B, 0x82);

    st25r3916ClrRegisterBits(ST25R3916_REG_AUX, ST25R3916_REG_AUX_dis_corr);
    st25r3916ChangeTestRegisterBits(0x04, 0x10, 0x10);

    uint16_t vdd_mV = 0;
    st25r3916AdjustRegulators(&vdd_mV);
    ESP_LOGI(TAG, "regulators adjusted, Vdd=%u mV", vdd_mV);
    msleep(6);

    st25r3916ClearAndEnableInterrupts(
        ST25R3916_IRQ_MASK_TXE | ST25R3916_IRQ_MASK_RXS |
        ST25R3916_IRQ_MASK_RXE | ST25R3916_IRQ_MASK_FWL |
        ST25R3916_IRQ_MASK_NRE | ST25R3916_IRQ_MASK_PAR |
        ST25R3916_IRQ_MASK_CRC | ST25R3916_IRQ_MASK_ERR1 |
        ST25R3916_IRQ_MASK_ERR2 | ST25R3916_IRQ_MASK_EON |
        ST25R3916_IRQ_MASK_EOF | ST25R3916_IRQ_MASK_COL |
        ST25R3916_IRQ_MASK_WU_A | ST25R3916_IRQ_MASK_WU_A_X |
        ST25R3916_IRQ_MASK_RXE_PTA);
}

int nfc_calibrate_antenna(uint8_t *out_a, uint8_t *out_b)
{
    nfc_poller_field_on();
    msleep(10);

    struct st25r3916AatTuneResult result = {0};
    ReturnCode err = st25r3916AatTune(NULL, &result);
    nfc_poller_field_off();

    if(err != RFAL_ERR_NONE) {
        ESP_LOGE(TAG, "AAT tune failed: %d", err);
        return -EIO;
    }

    ESP_LOGI(TAG, "AAT result: A=0x%02X B=0x%02X amp=%u pha=%u (%u steps)",
             result.aat_a, result.aat_b, result.amp, result.pha,
             result.measureCnt);
    if(out_a) *out_a = result.aat_a;
    if(out_b) *out_b = result.aat_b;
    return 0;
}

int nfc_poller_init_chip(void)
{
    nfc_chip_config();
    ESP_LOGI(TAG, "NFC poller chip config done");
    return 0;
}

void nfc_poller_reset_to_a_defaults(void)
{
    st25r3916ModifyRegister(ST25R3916_REG_MODE,
                            ST25R3916_REG_MODE_om3 | ST25R3916_REG_MODE_om2 |
                            ST25R3916_REG_MODE_om1 | ST25R3916_REG_MODE_om0 |
                            ST25R3916_REG_MODE_tr_am | ST25R3916_REG_MODE_targ,
                            ST25R3916_REG_MODE_om_iso14443a);

    st25r3916ChangeRegisterBits(ST25R3916_REG_BIT_RATE,
                                ST25R3916_REG_BIT_RATE_txrate_mask |
                                ST25R3916_REG_BIT_RATE_rxrate_mask,
                                ST25R3916_REG_BIT_RATE_txrate_106 |
                                ST25R3916_REG_BIT_RATE_rxrate_106);

    st25r3916WriteRegister(ST25R3916_REG_STREAM_MODE, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_ISO14443B_1, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_ISO14443B_2, 0x00);

    st25r3916ChangeRegisterBits(ST25R3916_REG_AUX_MOD,
                                ST25R3916_REG_AUX_MOD_dis_reg_am |
                                ST25R3916_REG_AUX_MOD_lm_ext_pol |
                                ST25R3916_REG_AUX_MOD_lm_ext |
                                ST25R3916_REG_AUX_MOD_lm_dri |
                                ST25R3916_REG_AUX_MOD_res_am,
                                ST25R3916_REG_AUX_MOD_lm_ext |
                                ST25R3916_REG_AUX_MOD_lm_dri);

    st25r3916WriteRegister(ST25R3916_REG_RX_CONF1, 0x20);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF2, 0xC3);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF3, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_RX_CONF4, 0x00);
    st25r3916WriteRegister(ST25R3916_REG_CORR_CONF1, 0x77);
    st25r3916WriteRegister(ST25R3916_REG_CORR_CONF2, 0x00);

    st25r3916WriteRegister(ST25R3916_REG_OVERSHOOT_CONF1, 0x40);
    st25r3916WriteRegister(ST25R3916_REG_OVERSHOOT_CONF2, 0x03);
    st25r3916WriteRegister(ST25R3916_REG_UNDERSHOOT_CONF1, 0x40);
    st25r3916WriteRegister(ST25R3916_REG_UNDERSHOOT_CONF2, 0x03);

    st25r3916WriteRegister(ST25R3916_REG_FIELD_ON_GT, 0x00);
}

int nfc_poller_field_on(void)
{
    nfc_poller_reset_to_a_defaults();
    st25r3916SetRegisterBits(ST25R3916_REG_OP_CONTROL,
                             ST25R3916_REG_OP_CONTROL_en |
                             ST25R3916_REG_OP_CONTROL_rx_en |
                             ST25R3916_REG_OP_CONTROL_tx_en);
    msleep(10);
    return 0;
}

int nfc_poller_field_off(void)
{
    st25r3916ClrRegisterBits(ST25R3916_REG_OP_CONTROL,
                             ST25R3916_REG_OP_CONTROL_tx_en);
    return 0;
}

static int poller_anticol_select(uint8_t sel_cmd,
                                 uint8_t *uid_out, uint8_t *uid_len_out,
                                 uint8_t *sak_out)
{
    uint8_t rx[8];
    uint16_t rx_len = 0;

    uint8_t sdd[2] = { sel_cmd, ISO14443A_SEL_PAR_NVB2 };
    int ret = nfc_trx(sdd, 2, rx, sizeof(rx), &rx_len, SELECT_TIMEOUT_MS, false, false);
    if(ret < 0 || rx_len < 5) return -EIO;

    uint8_t bcc = rx[0] ^ rx[1] ^ rx[2] ^ rx[3];
    if(bcc != rx[4]) {
        ESP_LOGW(TAG, "BCC mismatch: %02x != %02x", bcc, rx[4]);
        return -EIO;
    }

    uint8_t sel[7] = { sel_cmd, ISO14443A_SEL_PAR_NVB7,
                       rx[0], rx[1], rx[2], rx[3], rx[4] };
    rx_len = 0;
    ret = nfc_trx(sel, 7, rx, sizeof(rx), &rx_len, SELECT_TIMEOUT_MS, true, true);
    if(ret < 0 || rx_len < 1) return -EIO;

    *sak_out = rx[0];

    if(sel[2] == ISO14443A_CT) {
        memcpy(uid_out + *uid_len_out, &sel[3], 3);
        *uid_len_out += 3;
        return 1;
    }

    memcpy(uid_out + *uid_len_out, &sel[2], 4);
    *uid_len_out += 4;
    return 0;
}

int nfc_poller_detect(struct iso14443a_card *card, uint16_t timeout_ms)
{
    if(card) memset(card, 0, sizeof(*card));

    int64_t deadline = uptime_ms() + timeout_ms;

    while(uptime_ms() < deadline) {
        uint8_t rx[4];
        uint16_t rx_len = 0;

        int ret = nfc_trx_short_frame(ST25R3916_CMD_TRANSMIT_WUPA,
                                      rx, sizeof(rx), &rx_len, REQA_TIMEOUT_MS);
        if(ret < 0 || rx_len < 2) {
            msleep(1);
            continue;
        }

        uint8_t atqa[2] = { rx[0], rx[1] };
        uint8_t uid[ISO14443A_MAX_UID_LEN];
        uint8_t uid_len = 0;
        uint8_t sak = 0;

        static const uint8_t sel_cmds[] = {
            ISO14443A_SEL_CMD_CL1, ISO14443A_SEL_CMD_CL2, ISO14443A_SEL_CMD_CL3,
        };

        bool ok = true;
        for(int cl = 0; cl < 3; cl++) {
            ret = poller_anticol_select(sel_cmds[cl], uid, &uid_len, &sak);
            if(ret < 0) { ok = false; break; }
            if(ret == 0) break;
        }
        if(!ok) continue;

        if(card) {
            card->atqa[0] = atqa[0];
            card->atqa[1] = atqa[1];
            card->sak = sak;
            if(uid_len > ISO14443A_MAX_UID_LEN) uid_len = ISO14443A_MAX_UID_LEN;
            memcpy(card->uid, uid, uid_len);
            card->uid_len = uid_len;
        }

        ESP_LOGD(TAG, "card detected: UID %02x%02x%02x%02x SAK=%02x",
                 uid[0], uid[1], uid[2], uid[3], sak);
        return 0;
    }

    return -ETIMEDOUT;
}

int nfc_poller_halt(void)
{
    uint8_t hlta[2] = { ISO14443A_HLTA_CMD1, ISO14443A_HLTA_CMD2 };
    uint8_t rx[4];
    uint16_t rx_len = 0;
    nfc_trx(hlta, 2, rx, sizeof(rx), &rx_len, 10, true, false);
    return 0;
}

int nfc_poller_reactivate(struct iso14443a_card *card)
{
    nfc_poller_halt();
    msleep(2);

    uint8_t rx[4];
    uint16_t rx_len = 0;
    nfc_trx_short_frame(ST25R3916_CMD_TRANSMIT_WUPA,
                        rx, sizeof(rx), &rx_len, REQA_TIMEOUT_MS);
    if(rx_len < 2) return -EIO;

    uint8_t uid[ISO14443A_MAX_UID_LEN];
    uint8_t uid_len = 0;
    uint8_t sak = 0;

    static const uint8_t sel_cmds[] = {
        ISO14443A_SEL_CMD_CL1, ISO14443A_SEL_CMD_CL2, ISO14443A_SEL_CMD_CL3,
    };

    for(int cl = 0; cl < 3; cl++) {
        int ret = poller_anticol_select(sel_cmds[cl], uid, &uid_len, &sak);
        if(ret < 0) return -EIO;
        if(ret == 0) break;
    }

    if(card) {
        card->atqa[0] = rx[0];
        card->atqa[1] = rx[1];
        card->sak = sak;
        if(uid_len > ISO14443A_MAX_UID_LEN) uid_len = ISO14443A_MAX_UID_LEN;
        memcpy(card->uid, uid, uid_len);
        card->uid_len = uid_len;
    }
    return 0;
}
