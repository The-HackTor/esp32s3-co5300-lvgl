#include <nfc.h>
#include "nfc_poller.h"
#include "st25r3916.h"
#include "st25r3916_com.h"
#include "esp_log.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "nfc_iso14443a";

int nfc_init(const struct spi_dt_spec *spi, const struct gpio_dt_spec *irq)
{
    (void)spi; (void)irq;
    return nfc_poller_init_chip();
}

int nfc_reinit(void)
{
    ReturnCode err = st25r3916Initialize();
    return (err == RFAL_ERR_NONE) ? 0 : -EIO;
}

int nfc_field_on(void)
{
    return nfc_poller_field_on();
}

int nfc_field_off(void)
{
    return nfc_poller_field_off();
}

int nfc_detect_card(struct iso14443a_card *card)
{
    return nfc_poller_detect(card, 3000);
}

int nfc_halt(void)
{
    return nfc_poller_halt();
}

int nfc_reactivate(struct iso14443a_card *card)
{
    return nfc_poller_reactivate(card);
}

void nfc_diag(char *buf, size_t len)
{
    uint8_t reg_ic = 0, reg_op = 0;
    st25r3916ReadRegister(ST25R3916_REG_IC_IDENTITY, &reg_ic);
    st25r3916ReadRegister(ST25R3916_REG_OP_CONTROL, &reg_op);
    snprintf(buf, len, "IC=0x%02X OP=0x%02X", reg_ic, reg_op);
    (void)TAG;
}
