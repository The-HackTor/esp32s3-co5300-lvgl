#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "store/ir_store.h"

#include <stdio.h>

#define UNIVERSAL_LIST_MAX 32

static char list_buf[UNIVERSAL_LIST_MAX][IR_REMOTE_NAME_MAX];
static size_t list_count;

static void remote_selected(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= list_count) return;

    char path[IR_REMOTE_PATH_MAX];
    if(ir_universal_path((IrUniversalCategory)app->universal_category,
                         list_buf[index], path, sizeof(path)) != ESP_OK) return;

    ir_remote_free(&app->current_remote);
    if(ir_remote_load(&app->current_remote, path) != ESP_OK) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Load Failed", COLOR_RED);
        view_popup_set_text(app->popup, list_buf[index]);
        view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
        return;
    }
    app->is_learning_new_remote = false;
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Remote);
}

void ir_scene_universal_category_on_enter(void *ctx)
{
    IrApp *app = ctx;

    list_count = 0;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    esp_err_t err = ir_universal_list(cat, list_buf, UNIVERSAL_LIST_MAX, &list_count);

    char header[40];
    snprintf(header, sizeof(header), "%s", ir_universal_category_label(cat));

    if(err != ESP_OK || list_count == 0) {
        char msg[96];
        snprintf(msg, sizeof(msg),
                 "Drop .ir files in /sdcard/ir/universal/%s/",
                 ir_universal_category_dirname(cat));
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_SD_CARD, COLOR_DIM);
        view_popup_set_header(app->popup, header, COLOR_SECONDARY);
        view_popup_set_text(app->popup, msg);
        view_dispatcher_switch_to_view(app->view_dispatcher, IrViewPopup);
        return;
    }

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, header, COLOR_ORANGE);
    for(uint32_t i = 0; i < list_count; i++) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_FILE, list_buf[i],
                              COLOR_CYAN, i, remote_selected, app);
    }
    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_UniversalCategory));

    view_dispatcher_switch_to_view(app->view_dispatcher, IrViewSubmenu);
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
