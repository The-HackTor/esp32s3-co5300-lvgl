#include "nfc_app.h"
#include "nfc_scenes.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/styles.h"
#include <subghz_hal.h>
#include <string.h>


static NfcApp app;

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_enter,
static const SceneOnEnterCb nfc_on_enter[] = {
#include "nfc_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_event,
static const SceneOnEventCb nfc_on_event[] = {
#include "nfc_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_exit,
static const SceneOnExitCb nfc_on_exit[] = {
#include "nfc_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers nfc_scene_handlers = {
    .on_enter  = nfc_on_enter,
    .on_event  = nfc_on_event,
    .on_exit   = nfc_on_exit,
    .scene_num = NFC_SCENE_COUNT,
};

static void on_init(void) {
    memset(&app, 0, sizeof(app));

    app.screen = lv_obj_create(NULL);
    lv_obj_add_style(app.screen, &style_screen, 0);
    nav_install_gesture(app.screen);

    app.submenu = view_submenu_alloc(app.screen);
    app.action  = view_action_alloc(app.screen);
    app.info    = view_info_alloc(app.screen);
    app.dialog  = view_dialog_alloc(app.screen);
    app.popup   = view_popup_alloc(app.screen);
    app.custom  = view_custom_alloc(app.screen);

    app.view_dispatcher = view_dispatcher_alloc(app.screen);
    view_dispatcher_add_view(app.view_dispatcher, NfcViewSubmenu, view_submenu_get_module(app.submenu));
    view_dispatcher_add_view(app.view_dispatcher, NfcViewAction,  view_action_get_module(app.action));
    view_dispatcher_add_view(app.view_dispatcher, NfcViewInfo,    view_info_get_module(app.info));
    view_dispatcher_add_view(app.view_dispatcher, NfcViewDialog,  view_dialog_get_module(app.dialog));
    view_dispatcher_add_view(app.view_dispatcher, NfcViewPopup,   view_popup_get_module(app.popup));
    view_dispatcher_add_view(app.view_dispatcher, NfcViewCustom,  view_custom_get_module(app.custom));

    view_dispatcher_set_scene_manager(app.view_dispatcher, &app.scene_mgr);
}

static void on_enter(void) {
    /* Selective reset: full memset would clobber the view module pointers
     * created once in on_init() and never reallocated. */
    app.dump_valid    = false;
    app.emulating     = false;
    app.nonce_count   = 0;
    app.selected_slot = -1;
    memset(&app.dump, 0, sizeof(app.dump));
    memset(app.nonces, 0, sizeof(app.nonces));

    /* Reset all view modules to clean state */
    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);

    subghz_hal_idle();

    scene_manager_init(&app.scene_mgr, &nfc_scene_handlers, &app);
    scene_manager_next_scene(&app.scene_mgr, nfc_SCENE_Main);
    lv_screen_load(app.screen);
}

static void on_leave(void) {
    scene_manager_stop(&app.scene_mgr);
    hw_nfc_stop_emulate();

    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);
}

static lv_obj_t *get_screen(void) {
    return app.screen;
}

static SceneManager *get_scene_manager(void) {
    return &app.scene_mgr;
}

NfcApp *nfc_app_get(void) {
    return &app;
}

void nfc_app_register(void) {
    static const AppDescriptor desc = {
        .id                = APP_ID_NFC,
        .name              = "NFC",
        .icon              = LV_SYMBOL_WIFI,
        .color             = {.blue = 0xAA, .green = 0xD4, .red = 0x00},
        .on_init           = on_init,
        .on_enter          = on_enter,
        .on_leave          = on_leave,
        .get_screen        = get_screen,
        .get_scene_manager = get_scene_manager,
    };
    app_manager_register(&desc);
}
