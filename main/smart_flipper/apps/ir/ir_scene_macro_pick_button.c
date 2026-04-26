#include "ir_app.h"
#include "ir_scenes.h"

void ir_scene_macro_pick_button_on_enter(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
}

bool ir_scene_macro_pick_button_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_pick_button_on_exit(void *ctx)
{
    (void)ctx;
}
