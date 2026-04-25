#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"

/*
 * Capture Result / Decoded -- contextual menu after successful capture.
 * Shows: Save, Replay, Waveform (like Flipper's receiver info).
 */

enum { IDX_SAVE, IDX_REPLAY, IDX_SEND, IDX_WAVEFORM };

static void submenu_cb(void *context, uint32_t index)
{
    SubghzApp *app = context;
    switch(index) {
        case IDX_SAVE:
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Save);
            break;
        case IDX_REPLAY:
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Replay);
            break;
        case IDX_SEND:
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_EncodeTx);
            break;
        case IDX_WAVEFORM:
            scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Waveform);
            break;
    }
}

void subghz_scene_decoded_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_submenu_reset(app->submenu);

    if(app->decoded_valid) {
        char hdr[64];
        lv_snprintf(hdr, sizeof(hdr), "%s  0x%lX",
                    app->last_decoded.protocol,
                    (unsigned long)app->last_decoded.data);
        view_submenu_set_header(app->submenu, hdr, COLOR_GREEN);
    } else {
        char hdr[48];
        lv_snprintf(hdr, sizeof(hdr), "Captured  %u samples",
                    (unsigned)app->raw.count);
        view_submenu_set_header(app->submenu, hdr, COLOR_CYAN);
    }

    view_submenu_add_item(app->submenu, LV_SYMBOL_SAVE, "Save",
                          COLOR_BLUE, IDX_SAVE, submenu_cb, app);
    view_submenu_add_item(app->submenu, LV_SYMBOL_UPLOAD, "Replay",
                          COLOR_ORANGE, IDX_REPLAY, submenu_cb, app);
    if(app->decoded_valid && app->last_decoded.protocol) {
        view_submenu_add_item(app->submenu, LV_SYMBOL_RIGHT, "Send",
                              COLOR_YELLOW, IDX_SEND, submenu_cb, app);
    }
    view_submenu_add_item(app->submenu, LV_SYMBOL_EYE_OPEN, "Waveform",
                          COLOR_GREEN, IDX_WAVEFORM, submenu_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_decoded_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_decoded_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
}
