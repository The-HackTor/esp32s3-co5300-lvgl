#include "scene_manager.h"
#include <string.h>

void scene_manager_init(SceneManager *sm, const SceneManagerHandlers *handlers, void *ctx)
{
    memset(sm, 0, sizeof(*sm));
    sm->handlers = handlers;
    sm->context  = ctx;
    sm->running  = true;
}

void scene_manager_stop(SceneManager *sm)
{
    sm->stack_size = 0;
    sm->running = false;
}

void scene_manager_next_scene(SceneManager *sm, uint32_t scene_id)
{
    if(!sm->running) return;
    if(sm->stack_size > 0) {
        uint32_t cur = sm->stack[sm->stack_size - 1];
        sm->handlers->on_exit[cur](sm->context);
    }
    if(sm->stack_size < SCENE_STACK_MAX) {
        sm->stack[sm->stack_size++] = scene_id;
    }
    sm->handlers->on_enter[scene_id](sm->context);
}

bool scene_manager_previous_scene(SceneManager *sm)
{
    if(!sm->running) return false;
    if(sm->stack_size <= 1) {
        if(sm->stack_size == 1) {
            uint32_t cur = sm->stack[0];
            sm->handlers->on_exit[cur](sm->context);
            sm->stack_size = 0;
        }
        return false;
    }
    uint32_t cur = sm->stack[sm->stack_size - 1];
    sm->handlers->on_exit[cur](sm->context);
    sm->stack_size--;
    uint32_t prev = sm->stack[sm->stack_size - 1];
    sm->handlers->on_enter[prev](sm->context);
    return true;
}

bool scene_manager_handle_back_event(SceneManager *sm)
{
    if(sm->stack_size == 0) return false;

    SceneEvent ev = {.type = SceneEventTypeBack, .event = 0};
    uint32_t cur = sm->stack[sm->stack_size - 1];
    bool consumed = sm->handlers->on_event[cur](sm->context, ev);
    if(!consumed) {
        consumed = scene_manager_previous_scene(sm);
    }
    return consumed;
}

bool scene_manager_handle_custom_event(SceneManager *sm, uint32_t event)
{
    if(sm->stack_size == 0) return false;

    SceneEvent ev = {.type = SceneEventTypeCustom, .event = event};
    uint32_t cur = sm->stack[sm->stack_size - 1];
    return sm->handlers->on_event[cur](sm->context, ev);
}

void scene_manager_handle_tick_event(SceneManager *sm)
{
    if(sm->stack_size == 0) return;

    SceneEvent ev = {.type = SceneEventTypeTick, .event = 0};
    uint32_t cur = sm->stack[sm->stack_size - 1];
    sm->handlers->on_event[cur](sm->context, ev);
}

bool scene_manager_search_and_switch_to_previous_scene(SceneManager *sm, uint32_t scene_id)
{
    if(!sm->running) return false;
    uint32_t cur = sm->stack_size > 0 ? sm->stack[sm->stack_size - 1] : 0;
    if(sm->stack_size > 0) {
        sm->handlers->on_exit[cur](sm->context);
    }

    while(sm->stack_size > 0) {
        if(sm->stack[sm->stack_size - 1] == scene_id) {
            sm->handlers->on_enter[scene_id](sm->context);
            return true;
        }
        uint32_t top = sm->stack[sm->stack_size - 1];
        sm->handlers->on_exit[top](sm->context);
        sm->stack_size--;
    }
    return false;
}

bool scene_manager_search_and_switch_to_previous_scene_one_of(
    SceneManager *sm, const uint32_t *scene_ids, size_t count)
{
    for(size_t i = 0; i < count; i++) {
        /* Check if this scene_id is on the stack (non-destructive scan) */
        for(uint32_t s = 0; s < sm->stack_size; s++) {
            if(sm->stack[s] == scene_ids[i]) {
                return scene_manager_search_and_switch_to_previous_scene(sm, scene_ids[i]);
            }
        }
    }
    return false;
}

bool scene_manager_has_previous_scene(const SceneManager *sm, uint32_t scene_id)
{
    for(uint32_t i = 0; i < sm->stack_size; i++) {
        if(sm->stack[i] == scene_id) return true;
    }
    return false;
}

void scene_manager_set_scene_state(SceneManager *sm, uint32_t scene_id, uint32_t state)
{
    if(scene_id < SCENE_STATE_MAX) {
        sm->scene_state[scene_id] = state;
    }
}

uint32_t scene_manager_get_scene_state(const SceneManager *sm, uint32_t scene_id)
{
    if(scene_id < SCENE_STATE_MAX) {
        return sm->scene_state[scene_id];
    }
    return 0;
}

uint32_t scene_manager_get_current_scene(const SceneManager *sm)
{
    if(sm->stack_size > 0) {
        return sm->stack[sm->stack_size - 1];
    }
    return 0;
}
