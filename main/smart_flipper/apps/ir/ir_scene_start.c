#include "ir_app.h"
#include "ir_scenes.h"
#include "app/app_manager.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/ir_protocol_color.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECENT_INDEX_BASE  0xFFFFFE00u
#define IDX_UNIVERSAL      0
#define IDX_LEARN          1
#define IDX_SAVED          2
#define IDX_HISTORY        3
#define IDX_MACROS         4
#define IDX_AC             5
#define IDX_SETTINGS       6

static IrRecent s_recents[IR_RECENTS_MAX];
static size_t   s_recent_count;

static void send_recent(const IrRecent *r, IrApp *app)
{
    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s", r->protocol);
    msg.address = r->address;
    msg.command = r->command;

    uint16_t *t  = NULL;
    size_t    n  = 0;
    uint32_t  hz = 38000;
    if(ir_codecs_encode(&msg, &t, &n, &hz) != ESP_OK) {
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_YELLOW);
        view_popup_set_header(app->popup, "Encoder Pending", COLOR_YELLOW);
        view_popup_set_text(app->popup, msg.protocol);
        view_popup_set_timeout(app->popup, 800, NULL, NULL);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);
        return;
    }
    hw_rgb_set(255, 0, 0);
    hw_ir_send_raw(t, n, hz);
    hw_rgb_off();
    free(t);
}

static void submenu_cb(void *context, uint32_t index)
{
    IrApp *app = context;

    if(index >= RECENT_INDEX_BASE) {
        size_t ri = index - RECENT_INDEX_BASE;
        if(ri < s_recent_count) send_recent(&s_recents[ri], app);
        return;
    }

    scene_manager_set_scene_state(&app->scene_mgr, ir_SCENE_Start, index);

    switch(index) {
    case IDX_UNIVERSAL:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Universal);
        break;
    case IDX_LEARN:
        ir_remote_free(&app->current_remote);
        ir_remote_init(&app->current_remote, "");
        app->is_learning_new_remote = true;
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Learn);
        break;
    case IDX_SAVED:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_RemoteList);
        break;
    case IDX_HISTORY:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_History);
        break;
    case IDX_MACROS:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_MacroList);
        break;
    case IDX_AC:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_AcBrand);
        break;
    case IDX_SETTINGS:
        scene_manager_next_scene(&app->scene_mgr, ir_SCENE_Settings);
        break;
    }
}

void ir_scene_start_on_enter(void *ctx)
{
    IrApp *app = ctx;

    s_recent_count = 0;
    ir_recents_read(s_recents, IR_RECENTS_MAX, &s_recent_count);

    char remote_names[1][IR_REMOTE_NAME_MAX];
    size_t remote_count = 0;
    ir_store_list_remotes(remote_names, 0, &remote_count);

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "IR Remote", COLOR_ORANGE);

    for(size_t i = 0; i < s_recent_count; i++) {
        char label[40];
        snprintf(label, sizeof(label), "%s  %s 0x%lX",
                 s_recents[i].label, s_recents[i].protocol,
                 (unsigned long)s_recents[i].command);
        view_submenu_add_item(app->submenu, LV_SYMBOL_PLAY, label,
                              ir_protocol_color(s_recents[i].protocol),
                              (uint32_t)(RECENT_INDEX_BASE + i),
                              submenu_cb, app);
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Universal Remotes",
                          COLOR_ORANGE, IDX_UNIVERSAL, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_DOWNLOAD, "Learn New Remote",
                          COLOR_BLUE, IDX_LEARN, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SD_CARD, "Saved Remotes",
                          COLOR_CYAN, IDX_SAVED, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "History",
                          COLOR_GREEN, IDX_HISTORY, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SHUFFLE, "Macros",
                          COLOR_YELLOW, IDX_MACROS, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_REFRESH, "AC Remote",
                          COLOR_CYAN, IDX_AC, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_SETTINGS, "Settings",
                          COLOR_DIM, IDX_SETTINGS, submenu_cb, app);

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, ir_SCENE_Start));

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewSubmenu,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_start_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    if(event.type == SceneEventTypeBack) {
        app_manager_exit_current();
        return true;
    }
    return false;
}

void ir_scene_start_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
