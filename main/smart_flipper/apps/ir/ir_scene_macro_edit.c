#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"

#include <stdio.h>
#include <string.h>

#define ADD_STEP_INDEX     0xFFFFFFFEu
#define RUN_INDEX          0xFFFFFFFDu
#define DELETE_MACRO_INDEX 0xFFFFFFFCu

static int s_dialog_step = -1;

static void render(IrApp *app);

static void step_dialog_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(s_dialog_step < 0 || (size_t)s_dialog_step >= app->current_macro.step_count) {
        render(app);
        return;
    }
    size_t idx = (size_t)s_dialog_step;

    if(result == VIEW_DIALOG_RESULT_LEFT) {
        if(idx > 0) macro_move_step(&app->current_macro, idx, idx - 1);
    } else if(result == VIEW_DIALOG_RESULT_CENTER) {
        if(idx + 1 < app->current_macro.step_count)
            macro_move_step(&app->current_macro, idx, idx + 1);
    } else {
        macro_delete_step(&app->current_macro, idx);
    }
    macro_save(&app->current_macro);
    s_dialog_step = -1;
    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideRight, 120);
}

static void delete_macro_cb(void *ctx, uint32_t result)
{
    IrApp *app = ctx;
    if(result != VIEW_DIALOG_RESULT_LEFT) {
        render(app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                                (uint32_t)TransitionSlideRight, 120);
        return;
    }
    macro_delete_file(&app->current_macro);
    macro_free(&app->current_macro);
    scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                     ir_SCENE_MacroList);
}

static void step_tapped(void *ctx, uint32_t index)
{
    IrApp *app = ctx;

    if(index == ADD_STEP_INDEX) {
        memset(&app->pending_step, 0, sizeof(app->pending_step));
        app->pending_step.repeat = 1;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroPickRemote);
        return;
    }
    if(index == RUN_INDEX) {
        if(app->current_macro.step_count == 0) return;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroRun);
        return;
    }
    if(index == DELETE_MACRO_INDEX) {
        char text[96];
        snprintf(text, sizeof(text), "%s.macro", app->current_macro.name);
        view_dialog_reset(app->dialog);
        view_dialog_set_icon(app->dialog, LV_SYMBOL_WARNING, COLOR_RED);
        view_dialog_set_header(app->dialog, "Delete Macro?", COLOR_RED);
        view_dialog_set_text(app->dialog, text);
        view_dialog_set_left_button(app->dialog,  "Delete", COLOR_RED);
        view_dialog_set_right_button(app->dialog, "Keep",   COLOR_GREEN);
        view_dialog_set_callback(app->dialog, delete_macro_cb, app);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewDialog,
                                                (uint32_t)TransitionSlideLeft, 180);
        return;
    }
    if(index >= app->current_macro.step_count) return;

    s_dialog_step = (int)index;
    const IrMacroStep *s = &app->current_macro.steps[index];

    char header[64];
    char body[96];
    snprintf(header, sizeof(header), "Step %lu", (unsigned long)(index + 1));
    snprintf(body, sizeof(body), "%s / %s  %lu ms",
             s->remote, s->button, (unsigned long)s->delay_ms);

    view_dialog_reset(app->dialog);
    view_dialog_set_icon(app->dialog, LV_SYMBOL_SHUFFLE, COLOR_YELLOW);
    view_dialog_set_header(app->dialog, header, COLOR_YELLOW);
    view_dialog_set_text(app->dialog, body);
    view_dialog_set_left_button(app->dialog,   "Up",     COLOR_BLUE);
    view_dialog_set_center_button(app->dialog, "Down",   COLOR_BLUE);
    view_dialog_set_right_button(app->dialog,  "Delete", COLOR_RED);
    view_dialog_set_callback(app->dialog, step_dialog_cb, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewDialog,
                                            (uint32_t)TransitionSlideLeft, 180);
}

static void render(IrApp *app)
{
    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, app->current_macro.name, COLOR_YELLOW);

    if(app->current_macro.step_count == 0) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_WARNING, "No Steps",
                              COLOR_DIM, 0xFFFFFFFFu, NULL, NULL);
    } else {
        char label[64];
        for(size_t i = 0; i < app->current_macro.step_count; i++) {
            const IrMacroStep *s = &app->current_macro.steps[i];
            snprintf(label, sizeof(label), "%s/%s +%lums",
                     s->remote, s->button, (unsigned long)s->delay_ms);
            view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, label,
                                  COLOR_CYAN, (uint32_t)i, step_tapped, app);
        }
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_PLUS, "Add Step",
                          COLOR_GREEN, ADD_STEP_INDEX, step_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, "Run",
                          COLOR_RED, RUN_INDEX, step_tapped, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_TRASH, "Delete Macro",
                          COLOR_RED, DELETE_MACRO_INDEX, step_tapped, app);
}

void ir_scene_macro_edit_on_enter(void *ctx)
{
    IrApp *app = ctx;
    s_dialog_step = -1;
    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_macro_edit_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_edit_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_dialog_reset(app->dialog);
    view_popup_reset(app->popup);
    s_dialog_step = -1;
}
