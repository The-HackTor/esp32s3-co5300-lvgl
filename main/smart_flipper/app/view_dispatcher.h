#ifndef VIEW_DISPATCHER_H
#define VIEW_DISPATCHER_H

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>
#include "ui/view_module.h"
#include "scene_manager.h"

#define VIEW_DISPATCHER_MAX_VIEWS 16
#define VIEW_NONE                 UINT32_MAX

typedef struct ViewDispatcher ViewDispatcher;

ViewDispatcher *view_dispatcher_alloc(lv_obj_t *screen);
void            view_dispatcher_free(ViewDispatcher *vd);

void view_dispatcher_add_view(ViewDispatcher *vd, uint32_t view_id, ViewModule module);
void view_dispatcher_remove_view(ViewDispatcher *vd, uint32_t view_id);
void view_dispatcher_switch_to_view(ViewDispatcher *vd, uint32_t view_id);
void view_dispatcher_switch_to_view_animated(ViewDispatcher *vd, uint32_t view_id,
                                              uint32_t transition_type, uint32_t duration_ms);
uint32_t view_dispatcher_get_current_view(const ViewDispatcher *vd);

void view_dispatcher_set_scene_manager(ViewDispatcher *vd, SceneManager *sm);
void view_dispatcher_send_custom_event(ViewDispatcher *vd, uint32_t event);

#endif
