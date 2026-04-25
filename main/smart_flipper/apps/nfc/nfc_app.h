#ifndef NFC_APP_H
#define NFC_APP_H

#include <lvgl.h>
#include "app/scene_manager.h"
#include "app/view_dispatcher.h"
#include "ui/modules/view_submenu.h"
#include "ui/modules/view_action.h"
#include "ui/modules/view_info.h"
#include "ui/modules/view_dialog.h"
#include "ui/modules/view_popup.h"
#include "ui/modules/view_custom.h"
#include "hw/hw_types.h"
#include "hw/hw_nfc.h"

typedef enum {
    NfcViewSubmenu,
    NfcViewAction,
    NfcViewInfo,
    NfcViewDialog,
    NfcViewPopup,
    NfcViewCustom,
} NfcViewId;

typedef struct {
    lv_obj_t       *screen;
    ViewDispatcher *view_dispatcher;
    SceneManager    scene_mgr;

    /* Pre-allocated view modules */
    ViewSubmenu    *submenu;
    ViewAction     *action;
    ViewInfo       *info;
    ViewDialog     *dialog;
    ViewPopup      *popup;
    ViewCustom     *custom;

    /* App state */
    enum nfc_card_type card_type;
    struct mfc_dump      dump;
    struct mfu_dump      mfu_dump;
    bool            dump_valid;
    int             selected_slot;
    bool            emulating;
    struct mfkey_nonce     nonces[MFKEY_MAX_NONCES];
    uint8_t         nonce_count;

    /* MFKey: nonce pair tracking for key extraction */
    #define MFKEY_MAX_PAIRS 10
    struct {
        uint8_t  sector;
        uint8_t  key_type;    /* 0=A, 1=B */
        bool     complete;
        uint32_t nt0, nr0, ar0;
        uint32_t nt1, nr1, ar1;
    } nonce_pairs[MFKEY_MAX_PAIRS];
    uint8_t         nonce_pair_count;
    struct mfc_dump      detect_dump;  /* fake card used for detection */

    /* Recovered keys from mfkey32 or nested/hardnested attack */
    #define MAX_RECOVERED_KEYS 40
    struct {
        uint8_t  sector;
        uint8_t  key_type;
        uint8_t  key[6];
    } recovered_keys[MAX_RECOVERED_KEYS];
    uint8_t         recovered_key_count;

    /* Currently-attacked sector/key, set before nested/hardnested callback
     * fires so the cb can record the recovery against the right target. */
    uint8_t         attack_target_sector;
    uint8_t         attack_target_key_type;
} NfcApp;

void    nfc_app_register(void);
NfcApp *nfc_app_get(void);

#endif
