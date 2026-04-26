#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "lib/infrared/ac/ac_brand.h"

static void brand_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= ac_brand_count) return;
    if(app->ac_brand != ac_brand_table[index]) {
        app->ac_state.temp_c = 0;   /* Force ir_scene_ac to re-init defaults. */
        app->ac_brand = ac_brand_table[index];
    }
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_AcBrand, index);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Ac);
}

void ir_scene_ac_brand_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "AC Brand", COLOR_CYAN);

    for(size_t i = 0; i < ac_brand_count; i++) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH,
                              ac_brand_table[i]->name,
                              COLOR_CYAN, (uint32_t)i, brand_tapped, app);
    }
    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_AcBrand));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_ac_brand_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_ac_brand_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
