#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#define TAG "ir_univ_cat"

static void button_selected(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    if(ir_universal_db_signal_count(cat, index) == 0) return;

    app->univ_button_idx = (int)index;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_UniversalCategory, index);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_UniversalBrute);
}

void ir_scene_universal_category_on_enter(void *ctx)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    size_t total_buttons = ir_universal_db_button_count(cat);
    ESP_LOGI(TAG, "on_enter cat=%d buttons=%u", (int)cat, (unsigned)total_buttons);

    char header[40];
    snprintf(header, sizeof(header), "%s", ir_universal_category_label(cat));

    if(total_buttons == 0) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
        view_popup_set_header(app->popup, header, COLOR_SECONDARY);
        view_popup_set_text(app->popup, "Universal DB empty.");
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, header, COLOR_ORANGE);
    for(size_t i = 0; i < total_buttons; i++) {
        const char *name = ir_universal_db_button_name(cat, i);
        size_t n = ir_universal_db_signal_count(cat, i);
        char label[40];
        snprintf(label, sizeof(label), "%s  (%u)", name, (unsigned)n);
        view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, label,
                              COLOR_RED, (uint32_t)i, button_selected, app);
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_UniversalCategory));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_universal_category_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_universal_category_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
