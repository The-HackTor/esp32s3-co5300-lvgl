#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SCENE_STACK_MAX  16
#define SCENE_STATE_MAX  32

typedef enum {
    SceneEventTypeCustom,
    SceneEventTypeBack,
    SceneEventTypeTick,
} SceneEventType;

typedef struct {
    SceneEventType type;
    uint32_t       event;
} SceneEvent;

typedef void (*SceneOnEnterCb)(void *ctx);
typedef bool (*SceneOnEventCb)(void *ctx, SceneEvent event);
typedef void (*SceneOnExitCb)(void *ctx);

typedef struct {
    const SceneOnEnterCb *on_enter;
    const SceneOnEventCb *on_event;
    const SceneOnExitCb  *on_exit;
    uint32_t              scene_num;
} SceneManagerHandlers;

typedef struct {
    uint32_t                    stack[SCENE_STACK_MAX];
    uint32_t                    stack_size;
    uint32_t                    scene_state[SCENE_STATE_MAX];
    const SceneManagerHandlers *handlers;
    void                       *context;
    bool                        running;
} SceneManager;

void     scene_manager_init(SceneManager *sm, const SceneManagerHandlers *handlers, void *ctx);
void     scene_manager_stop(SceneManager *sm);
void     scene_manager_next_scene(SceneManager *sm, uint32_t scene_id);
bool     scene_manager_previous_scene(SceneManager *sm);
bool     scene_manager_handle_back_event(SceneManager *sm);
void     scene_manager_handle_custom_event(SceneManager *sm, uint32_t event);
void     scene_manager_handle_tick_event(SceneManager *sm);
bool     scene_manager_search_and_switch_to_previous_scene(SceneManager *sm, uint32_t scene_id);
bool     scene_manager_search_and_switch_to_previous_scene_one_of(
             SceneManager *sm, const uint32_t *scene_ids, size_t count);
bool     scene_manager_has_previous_scene(const SceneManager *sm, uint32_t scene_id);
void     scene_manager_set_scene_state(SceneManager *sm, uint32_t scene_id, uint32_t state);
uint32_t scene_manager_get_scene_state(const SceneManager *sm, uint32_t scene_id);
uint32_t scene_manager_get_current_scene(const SceneManager *sm);

#endif
