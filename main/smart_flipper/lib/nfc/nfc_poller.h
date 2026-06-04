#ifndef NFC_POLLER_H
#define NFC_POLLER_H

#include <nfc.h>

int  nfc_poller_init_chip(void);
void nfc_chip_config(void);
int  nfc_calibrate_antenna(uint8_t *out_a, uint8_t *out_b);
int  nfc_poller_field_on(void);
int  nfc_poller_field_off(void);
int  nfc_poller_detect(struct iso14443a_card *card, uint16_t timeout_ms);
int  nfc_poller_halt(void);
int  nfc_poller_reactivate(struct iso14443a_card *card);
void nfc_poller_reset_to_a_defaults(void);

#endif
