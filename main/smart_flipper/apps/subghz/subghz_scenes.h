#ifndef SUBGHZ_SCENES_H
#define SUBGHZ_SCENES_H

#include <stdbool.h>
#include "app/scene_manager.h"

#define ADD_SCENE(prefix, name, id) prefix##_SCENE_##id,
typedef enum {
#include "subghz_scene_config.h"
    subghz_SCENE_COUNT
} SubghzSceneId;
#undef ADD_SCENE

#define SUBGHZ_SCENE_COUNT subghz_SCENE_COUNT

#define ADD_SCENE(prefix, name, id)                                        \
    void prefix##_scene_##name##_on_enter(void *ctx);                      \
    bool prefix##_scene_##name##_on_event(void *ctx, SceneEvent event);    \
    void prefix##_scene_##name##_on_exit(void *ctx);
#include "subghz_scene_config.h"
#undef ADD_SCENE

enum {
    SUBGHZ_EVT_CAPTURE_DONE = 0x200,
    SUBGHZ_EVT_REPLAY_DONE,
    SUBGHZ_EVT_DECODED,
    SUBGHZ_EVT_FREQ_HIT,
    SUBGHZ_EVT_PING_DONE,
    SUBGHZ_EVT_RELAY_PROGRESS,
    SUBGHZ_EVT_RELAY_DONE,
    SUBGHZ_EVT_READ_UPDATE,
    SUBGHZ_EVT_HOP_CHANGE,
    SUBGHZ_EVT_ENCODE_TX_DONE,
};

extern const SceneManagerHandlers subghz_scene_handlers;

#endif
