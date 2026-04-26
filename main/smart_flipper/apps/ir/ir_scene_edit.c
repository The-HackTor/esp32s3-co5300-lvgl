#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>
#include <string.h>

enum {
    EDIT_IDX_RENAME_REMOTE = 0,
    EDIT_IDX_RENAME_BUTTON,
    EDIT_IDX_DELETE_BUTTON,
    EDIT_IDX_MOVE_BUTTON,
    EDIT_IDX_DELETE_REMOTE,
};

static void item_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_Edit, index);

    switch(index) {
    case EDIT_IDX_RENAME_REMOTE:
        app->edit_op = IR_EDIT_OP_RENAME_REMOTE;
        snprintf(app->name_buffer, sizeof(app->name_buffer), "%s",
                 app->current_remote.name);
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditRename);
        break;
    case EDIT_IDX_RENAME_BUTTON:
        app->edit_op = IR_EDIT_OP_RENAME_BUTTON;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditButtonSelect);
        break;
    case EDIT_IDX_DELETE_BUTTON:
        app->edit_op = IR_EDIT_OP_DELETE_BUTTON;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditButtonSelect);
        break;
    case EDIT_IDX_MOVE_BUTTON:
        app->edit_op = IR_EDIT_OP_MOVE_BUTTON;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditButtonSelect);
        break;
    case EDIT_IDX_DELETE_REMOTE:
        app->edit_op = IR_EDIT_OP_DELETE_REMOTE;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditDelete);
        break;
    }
}

void ir_scene_edit_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Edit", COLOR_YELLOW);

    view_submenu_add_item(app->submenu, LV_SYMBOL_EDIT, "Rename Remote",
                          COLOR_CYAN, EDIT_IDX_RENAME_REMOTE, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_EDIT, "Rename Button",
                          COLOR_CYAN, EDIT_IDX_RENAME_BUTTON, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Delete Button",
                          COLOR_ORANGE, EDIT_IDX_DELETE_BUTTON, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SHUFFLE, "Move Button",
                          COLOR_BLUE, EDIT_IDX_MOVE_BUTTON, item_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Delete Remote",
                          COLOR_RED, EDIT_IDX_DELETE_REMOTE, item_tapped, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_Edit));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_edit_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_edit_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
