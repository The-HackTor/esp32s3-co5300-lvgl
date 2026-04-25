#ifndef VIEW_CUSTOM_H
#define VIEW_CUSTOM_H

#include <lvgl.h>
#include "ui/view_module.h"

/*
 * A lightweight view module wrapping a bare lv_obj_t.
 * For complex one-off scenes (analyzer, waveform, etc.) that need
 * custom LVGL trees but still want dispatcher hide/show behavior.
 *
 * Scenes populate the container's children in on_enter and
 * call view_custom_clean() in on_exit.
 */
typedef struct ViewCustom ViewCustom;

ViewCustom *view_custom_alloc(lv_obj_t *parent);
void        view_custom_free(ViewCustom *vc);
ViewModule  view_custom_get_module(ViewCustom *vc);
lv_obj_t   *view_custom_get_view(ViewCustom *vc);

/* Remove all children but keep the root container alive */
void view_custom_clean(ViewCustom *vc);

#endif
