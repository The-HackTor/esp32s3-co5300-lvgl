#ifndef HW_NFC_H
#define HW_NFC_H

#include "hw_types.h"
#include <nfc.h>
#include <mf_ultralight.h>
#include <mf_classic_mfkey32.h>
#include <mf_classic_nested.h>
#include <mf_classic_hardnested.h>
#include <stdbool.h>
#include <stddef.h>

typedef void (*hw_nfc_read_cb_t)(bool success, enum nfc_card_type type, void *ctx);
typedef void (*hw_nfc_nonce_cb_t)(const struct mfkey_nonce *nonce, void *ctx);
typedef void (*hw_nfc_mfkey32_cb_t)(const struct mfkey32_result *results,
                                    size_t count, void *ctx);
typedef void (*hw_nfc_nested_cb_t)(bool success, const struct nested_result *res,
                                   void *ctx);
typedef void (*hw_nfc_hardnested_cb_t)(bool success,
                                       const struct hardnested_result *res,
                                       void *ctx);

void hw_nfc_init(bool nfc_available);

/* Fills exactly one of mfc_out / mfu_out; cb runs in LVGL context, `type`
 * picks the live buffer. */
void hw_nfc_start_read(struct mfc_dump *mfc_out, struct mfu_dump *mfu_out,
                       hw_nfc_read_cb_t cb, void *ctx);
void hw_nfc_cancel_read(void);

/* `src` must outlive hw_nfc_stop_emulate(). */
void hw_nfc_start_emulate(struct mfc_dump *src, hw_nfc_nonce_cb_t cb, void *ctx);
void hw_nfc_start_emulate_mfu(struct mfu_dump *src);
void hw_nfc_stop_emulate(void);
bool hw_nfc_is_emulating(void);

void hw_nfc_solve_mfkey32(const struct mfkey32_nonce_pair *pairs, size_t count,
                          hw_nfc_mfkey32_cb_t cb, void *ctx);
void hw_nfc_start_nested(const struct nested_params *params,
                         hw_nfc_nested_cb_t cb, void *ctx);
void hw_nfc_start_hardnested(const struct nested_params *params,
                             hw_nfc_hardnested_cb_t cb, void *ctx);

void hw_nfc_generate_fake_dump(struct mfc_dump *dump);

#endif
