#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>

enum {
    MOVE_IDX_UP = 0,
    MOVE_IDX_DOWN,
    MOVE_IDX_DONE,
};

static void render(IrApp *app);

static void item_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    int cur = app->selected_button_idx;
    if(cur < 0 || (size_t)cur >= app->current_remote.button_count) return;

    if(index == MOVE_IDX_UP && cur > 0) {
        if(ir_remote_move_button(&app->current_remote,
                                 (size_t)cur, (size_t)(cur - 1)) == ESP_OK) {
            app->selected_button_idx = cur - 1;
            ir_remote_save(&app->current_remote);
            render(app);
        }
    } else if(index == MOVE_IDX_DOWN
              && (size_t)(cur + 1) < app->current_remote.button_count) {
        if(ir_remote_move_button(&app->current_remote,
                                 (size_t)cur, (size_t)(cur + 1)) == ESP_OK) {
            app->selected_button_idx = cur + 1;
            ir_remote_save(&app->current_remote);
            render(app);
        }
    } else if(index == MOVE_IDX_DONE) {
        scene_manager_search_and_switch_to_previous_scene(
            &app->scene_mgr, ir_SCENE_Remote);
    }
}

static void render(IrApp *app)
{
    char header[48];
    int cur = app->selected_button_idx;
    if(cur < 0 || (size_t)cur >= app->current_remote.button_count) {
        snprintf(header, sizeof(header), "Move");
    } else {
        snprintf(header, sizeof(header), "%s (%d/%u)",
                 app->current_remote.buttons[cur].name,
                 cur + 1, (unsigned)app->current_remote.button_count);
    }

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, header, COLOR_BLUE);

    bool can_up   = (cur > 0);
    bool can_down = (cur >= 0 &&
                    (size_t)(cur + 1) < app->current_remote.button_count);

    view_submenu_add_item(app->submenu, LV_SYMBOL_UP, "Move Up",
                          can_up ? COLOR_BLUE : COLOR_DIM,
                          MOVE_IDX_UP, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWN, "Move Down",
                          can_down ? COLOR_BLUE : COLOR_DIM,
                          MOVE_IDX_DOWN, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_OK, "Done",
                          COLOR_GREEN, MOVE_IDX_DONE, item_tapped, app);
}

void ir_scene_edit_move_on_enter(void *ctx)
{
    IrApp *app = ctx;
    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_edit_move_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_edit_move_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
