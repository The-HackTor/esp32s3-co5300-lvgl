#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>
#include <string.h>

#define MACRO_LIST_VIEW_MAX 16
#define CREATE_INDEX        0xFFFFFFFEu

static char  list_buf[MACRO_LIST_VIEW_MAX][IR_MACRO_NAME_MAX];
static size_t list_count;

static void name_accepted(void *ctx, const char *text)
{
    IrApp *app = ctx;
    snprintf(app->macro_name_buf, sizeof(app->macro_name_buf), "%s", text);
    scene_manager_handle_custom_event(&app->scene_mgr, IR_EVT_MACRO_NAME_ACCEPTED);
}

static void item_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;

    if(index == CREATE_INDEX) {
        memset(app->macro_name_buf, 0, sizeof(app->macro_name_buf));
        view_text_input_reset(app->text_input);
        view_text_input_set_header(app->text_input, "Macro Name", COLOR_YELLOW);
        view_text_input_set_buffer(app->text_input, app->macro_name_buf,
                                   sizeof(app->macro_name_buf));
        view_text_input_set_callback(app->text_input, name_accepted, app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewTextInput,
                                                (uint32_t)TransitionSlideLeft, 180);
        return;
    }
    if(index >= list_count) return;

    char path[IR_MACRO_PATH_MAX];
    if(macro_store_path(path, sizeof(path), list_buf[index]) != ESP_OK) return;

    macro_free(&app->current_macro);
    if(macro_load(&app->current_macro, path) != ESP_OK) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
        view_popup_set_header(app->popup, "Load Failed", COLOR_RED);
        view_popup_set_text(app->popup, list_buf[index]);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_MacroList, index);
    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroEdit);
}

void ir_scene_macro_list_on_enter(void *ctx)
{
    IrApp *app = ctx;

    list_count = 0;
    macro_store_list(list_buf, MACRO_LIST_VIEW_MAX, &list_count);

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Macros", COLOR_YELLOW);

    if(list_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Macros",
                              COLOR_DIM, 0xFFFFFFFFu, NULL, NULL);
    } else {
        for(size_t i = 0; i < list_count; i++) {
            view_submenu_add_item(app->submenu, LV_SYMBOL_SHUFFLE, list_buf[i],
                                  COLOR_YELLOW, (uint32_t)i, item_tapped, app);
        }
    }
    view_submenu_add_item(app->submenu, LV_SYMBOL_PLUS, "Create New",
                          COLOR_GREEN, CREATE_INDEX, item_tapped, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_MacroList));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_list_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom && event.event == IR_EVT_MACRO_NAME_ACCEPTED) {
        if(app->macro_name_buf[0] == '\0') {
            scene_manager_previous_scene(&app->scene_mgr);
            return true;
        }
        macro_free(&app->current_macro);
        if(macro_init(&app->current_macro, app->macro_name_buf) != ESP_OK) {
            scene_manager_previous_scene(&app->scene_mgr);
            return true;
        }
        macro_save(&app->current_macro);
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroEdit);
        return true;
    }
    return false;
}

void ir_scene_macro_list_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_text_input_reset(app->text_input);
    view_popup_reset(app->popup);
}
