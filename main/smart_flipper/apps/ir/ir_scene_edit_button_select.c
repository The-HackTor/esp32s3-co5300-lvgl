#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "lib/infrared/ir_protocol_color.h"

#include <stdio.h>

static void btn_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;
    if(index >= app->current_remote.button_count) return;
    app->selected_button_idx = (int)index;
    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_EditButtonSelect, index);

    switch((IrEditOp)app->edit_op) {
    case IR_EDIT_OP_RENAME_BUTTON:
        snprintf(app->name_buffer, sizeof(app->name_buffer), "%s",
                 app->current_remote.buttons[index].name);
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditRename);
        break;
    case IR_EDIT_OP_DELETE_BUTTON:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditDelete);
        break;
    case IR_EDIT_OP_MOVE_BUTTON:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_EditMove);
        break;
    default:
        scene_manager_previous_scene(&app->scene_mgr);
        break;
    }
}

void ir_scene_edit_button_select_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_submenu_reset(app->submenu);

    const char *header;
    switch((IrEditOp)app->edit_op) {
    case IR_EDIT_OP_RENAME_BUTTON: header = "Rename Which?"; break;
    case IR_EDIT_OP_DELETE_BUTTON: header = "Delete Which?"; break;
    case IR_EDIT_OP_MOVE_BUTTON:   header = "Move Which?";   break;
    default:                       header = "Pick Button";   break;
    }
    view_submenu_set_header(app->submenu, header, COLOR_YELLOW);

    if(app->current_remote.button_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Buttons",
                              COLOR_DIM, 0, NULL, NULL);
    } else {
        for(size_t i = 0; i < app->current_remote.button_count; i++) {
            const IrButton *b = &app->current_remote.buttons[i];
            lv_color_t color = (b->signal.type == INFRARED_SIGNAL_PARSED)
                ? ir_protocol_color(b->signal.parsed.protocol)
                : COLOR_YELLOW;
            view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, b->name,
                                  color, (uint32_t)i, btn_tapped, app);
        }
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_EditButtonSelect));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_edit_button_select_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_edit_button_select_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
}
