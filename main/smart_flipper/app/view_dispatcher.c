#include "view_dispatcher.h"
#include "ui/transition.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t   view_id;
    ViewModule module;
    bool       used;
} ViewEntry;

struct ViewDispatcher {
    lv_obj_t     *screen;
    ViewEntry     views[VIEW_DISPATCHER_MAX_VIEWS];
    uint32_t      current_view_id;
    SceneManager *scene_manager;
};

ViewDispatcher *view_dispatcher_alloc(lv_obj_t *screen)
{
    ViewDispatcher *vd = calloc(1, sizeof(ViewDispatcher));
    vd->screen = screen;
    vd->current_view_id = VIEW_NONE;
    return vd;
}

void view_dispatcher_free(ViewDispatcher *vd)
{
    if(!vd) return;
    vd->scene_manager = NULL;  /* prevent event routing during teardown */
    vd->current_view_id = VIEW_NONE;
    for(int i = 0; i < VIEW_DISPATCHER_MAX_VIEWS; i++) {
        if(vd->views[i].used) {
            view_module_destroy(&vd->views[i].module);
            vd->views[i].used = false;
        }
    }
    free(vd);
}

static ViewEntry *find_entry(ViewDispatcher *vd, uint32_t view_id)
{
    for(int i = 0; i < VIEW_DISPATCHER_MAX_VIEWS; i++) {
        if(vd->views[i].used && vd->views[i].view_id == view_id) {
            return &vd->views[i];
        }
    }
    return NULL;
}

void view_dispatcher_add_view(ViewDispatcher *vd, uint32_t view_id, ViewModule module)
{
    /* View roots are already children of the app screen (created hidden in alloc).
     * Just register the module -- no reparenting needed. */
    for(int i = 0; i < VIEW_DISPATCHER_MAX_VIEWS; i++) {
        if(!vd->views[i].used) {
            vd->views[i].view_id = view_id;
            vd->views[i].module  = module;
            vd->views[i].used    = true;
            return;
        }
    }
}

void view_dispatcher_remove_view(ViewDispatcher *vd, uint32_t view_id)
{
    ViewEntry *entry = find_entry(vd, view_id);
    if(!entry) return;

    lv_obj_t *view = view_module_get_view(&entry->module);
    lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);

    if(vd->current_view_id == view_id) {
        vd->current_view_id = VIEW_NONE;
    }
    entry->used = false;
}

void view_dispatcher_switch_to_view(ViewDispatcher *vd, uint32_t view_id)
{
    if(!vd) return;
    /* Hide current view */
    if(vd->current_view_id != VIEW_NONE) {
        ViewEntry *cur = find_entry(vd, vd->current_view_id);
        if(cur) {
            lv_obj_t *cur_view = view_module_get_view(&cur->module);
            lv_obj_add_flag(cur_view, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Show new view */
    ViewEntry *next = find_entry(vd, view_id);
    if(next) {
        lv_obj_t *next_view = view_module_get_view(&next->module);
        lv_obj_remove_flag(next_view, LV_OBJ_FLAG_HIDDEN);
        vd->current_view_id = view_id;
    }
}

void view_dispatcher_switch_to_view_animated(ViewDispatcher *vd, uint32_t view_id,
                                              uint32_t transition_type, uint32_t duration_ms)
{
    lv_obj_t *outgoing = NULL;
    lv_obj_t *incoming = NULL;

    if(vd->current_view_id != VIEW_NONE) {
        ViewEntry *cur = find_entry(vd, vd->current_view_id);
        if(cur) outgoing = view_module_get_view(&cur->module);
    }

    ViewEntry *next = find_entry(vd, view_id);
    if(next) {
        incoming = view_module_get_view(&next->module);
        lv_obj_remove_flag(incoming, LV_OBJ_FLAG_HIDDEN);
        vd->current_view_id = view_id;
    }

    transition_apply(outgoing, incoming, (TransitionType)transition_type, duration_ms);
}

uint32_t view_dispatcher_get_current_view(const ViewDispatcher *vd)
{
    return vd->current_view_id;
}

void view_dispatcher_set_scene_manager(ViewDispatcher *vd, SceneManager *sm)
{
    vd->scene_manager = sm;
}

void view_dispatcher_send_custom_event(ViewDispatcher *vd, uint32_t event)
{
    if(!vd) return;
    if(vd->scene_manager) {
        scene_manager_handle_custom_event(vd->scene_manager, event);
    }
}
