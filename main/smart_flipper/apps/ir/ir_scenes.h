#ifndef IR_SCENES_H
#define IR_SCENES_H

#include <stdbool.h>
#include "app/scene_manager.h"

#define ADD_SCENE(prefix, name, id) prefix##_SCENE_##id,
typedef enum {
#include "ir_scene_config.h"
    ir_SCENE_COUNT
} IrSceneId;
#undef ADD_SCENE

#define IR_SCENE_COUNT ir_SCENE_COUNT

#define ADD_SCENE(prefix, name, id)                                        \
    void prefix##_scene_##name##_on_enter(void *ctx);                      \
    bool prefix##_scene_##name##_on_event(void *ctx, SceneEvent event);    \
    void prefix##_scene_##name##_on_exit(void *ctx);
#include "ir_scene_config.h"
#undef ADD_SCENE

enum {
    IR_EVT_RX_DECODED = 0x200,
    IR_EVT_RX_RAW,
    IR_EVT_NAME_ACCEPTED,
    IR_EVT_DISCARD_CONFIRMED,
    IR_EVT_EDIT_NAME_ACCEPTED,
    IR_EVT_EDIT_DELETE_CONFIRMED,
    IR_EVT_MACRO_NAME_ACCEPTED,
    IR_EVT_MACRO_STEP_DONE,
    IR_EVT_MACRO_FINISHED,
    IR_EVT_UNIV_SAVE_NAME_ACCEPTED,
    IR_EVT_DUPLICATE_NAME_ACCEPTED,
};

typedef enum {
    IR_EDIT_OP_NONE = 0,
    IR_EDIT_OP_RENAME_REMOTE,
    IR_EDIT_OP_RENAME_BUTTON,
    IR_EDIT_OP_DELETE_BUTTON,
    IR_EDIT_OP_MOVE_BUTTON,
    IR_EDIT_OP_DELETE_REMOTE,
} IrEditOp;

extern const SceneManagerHandlers ir_scene_handlers;

#endif
