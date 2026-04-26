#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "esp_log.h"

#define TAG "ir_universal"

static void category_selected(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= IR_UNIVERSAL_CAT_COUNT) return;
    ESP_LOGI(TAG, "category=%lu", (unsigned long)index);
    app->universal_category = (int)index;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_Universal, index);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_UniversalCategory);
}

void ir_scene_universal_on_enter(void *ctx)
{
    IrApp *app = ctx;
    ESP_LOGI(TAG, "on_enter");

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Universal Remotes", COLOR_ORANGE);

    static const char *icons[IR_UNIVERSAL_CAT_COUNT] = {
        LV_SYMBOL_VIDEO,
        LV_SYMBOL_REFRESH,
        LV_SYMBOL_AUDIO,
        LV_SYMBOL_IMAGE,
    };
    lv_color_t cat_color[IR_UNIVERSAL_CAT_COUNT] = {
        COLOR_BLUE, COLOR_CYAN, COLOR_ORANGE, COLOR_MAGENTA,
    };

    for(int i = 0; i < IR_UNIVERSAL_CAT_COUNT; i++) {
        view_submenu_add_item(app->submenu, icons[i],
                              ir_universal_category_label((IrUniversalCategory)i),
                              cat_color[i], (uint32_t)i, category_selected, app);
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_Universal));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_universal_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_universal_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
