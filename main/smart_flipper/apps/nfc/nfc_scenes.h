#ifndef NFC_SCENES_H
#define NFC_SCENES_H

#include <stdbool.h>
#include "app/scene_manager.h"

#define ADD_SCENE(prefix, name, id) prefix##_SCENE_##id,
typedef enum {
#include "nfc_scene_config.h"
    nfc_SCENE_COUNT
} NfcSceneId;
#undef ADD_SCENE

#define NFC_SCENE_COUNT nfc_SCENE_COUNT

#define ADD_SCENE(prefix, name, id)                                        \
    void prefix##_scene_##name##_on_enter(void *ctx);                      \
    bool prefix##_scene_##name##_on_event(void *ctx, SceneEvent event);    \
    void prefix##_scene_##name##_on_exit(void *ctx);
#include "nfc_scene_config.h"
#undef ADD_SCENE

enum {
    NFC_EVT_READ_DONE = 0x100,
    NFC_EVT_READ_FAIL,
    NFC_EVT_NONCE_CAPTURED,
    NFC_EVT_ITEM_SELECTED,
    NFC_EVT_SAVE_CARD,
    NFC_EVT_EMULATE_CARD,
    NFC_EVT_STOP_EMULATE,
    NFC_EVT_READER_DETECTED,
    NFC_EVT_READER_LOST,
    NFC_EVT_NONCE_PAIR_COMPLETE,
    NFC_EVT_MFKEY_DONE,
    NFC_EVT_MFKEY_SOLVE_DONE,
    NFC_EVT_MFKEY_SOLVE_FAIL,
    NFC_EVT_NESTED_DONE,
    NFC_EVT_NESTED_FAIL,
    NFC_EVT_HARDNESTED_DONE,
    NFC_EVT_HARDNESTED_FAIL,
};

/* --- Scene handler struct (defined in nfc_app.c) --- */
extern const SceneManagerHandlers nfc_scene_handlers;

#endif
