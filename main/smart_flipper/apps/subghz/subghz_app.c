#include "subghz_app.h"
#include "subghz_scenes.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/styles.h"
#include <st25r_zephyr.h>
#include <string.h>

static SubghzApp app;

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_enter,
static const SceneOnEnterCb subghz_on_enter[] = {
#include "subghz_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_event,
static const SceneOnEventCb subghz_on_event[] = {
#include "subghz_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_exit,
static const SceneOnExitCb subghz_on_exit[] = {
#include "subghz_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers subghz_scene_handlers = {
    .on_enter  = subghz_on_enter,
    .on_event  = subghz_on_event,
    .on_exit   = subghz_on_exit,
    .scene_num = SUBGHZ_SCENE_COUNT,
};

static void on_init(void)
{
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
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewSubmenu, view_submenu_get_module(app.submenu));
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewAction,  view_action_get_module(app.action));
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewInfo,    view_info_get_module(app.info));
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewDialog,  view_dialog_get_module(app.dialog));
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewPopup,   view_popup_get_module(app.popup));
    view_dispatcher_add_view(app.view_dispatcher, SubghzViewCustom,  view_custom_get_module(app.custom));

    view_dispatcher_set_scene_manager(app.view_dispatcher, &app.scene_mgr);
}

static void on_enter(void)
{
    /* memset would clobber view module pointers allocated in on_init */
    app.raw_valid      = false;
    app.decoded_valid  = false;
    app.frequency       = hw_subghz_get_frequency();
    if(app.frequency == 0) app.frequency = 433920;
    app.preset          = hw_subghz_get_preset();
    app.selected_slot   = 0;
    app.hopping         = false;
    app.rssi_threshold  = -120.0f;
    memset(&app.raw, 0, sizeof(app.raw));
    memset(&app.last_decoded, 0, sizeof(app.last_decoded));

    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);

    st25r_trigger_disable();

    scene_manager_init(&app.scene_mgr, &subghz_scene_handlers, &app);
    scene_manager_next_scene(&app.scene_mgr, subghz_SCENE_Main);
    lv_screen_load(app.screen);
}

static void on_leave(void)
{
    sim_subghz_stop_analyzer();
    sim_subghz_stop_decode();
    sim_subghz_stop_read();
    hw_subghz_stop_capture();
    hw_subghz_stop_replay();
    sim_subghz_stop_relay();
    scene_manager_stop(&app.scene_mgr);

    st25r_trigger_enable();

    /* free is owned by on_init's allocations -- only reset here */
    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);
}

static lv_obj_t *get_screen(void)
{
    return app.screen;
}

static SceneManager *get_scene_manager(void)
{
    return &app.scene_mgr;
}

SubghzApp *subghz_app_get(void)
{
    return &app;
}

void subghz_app_register(void)
{
    static const AppDescriptor desc = {
        .id                = APP_ID_SUBGHZ,
        .name              = "SubGHz",
        .icon              = LV_SYMBOL_WIFI,
        .color             = {.blue = 0x35, .green = 0x6B, .red = 0xFF},
        .on_init           = on_init,
        .on_enter          = on_enter,
        .on_leave          = on_leave,
        .get_screen        = get_screen,
        .get_scene_manager = get_scene_manager,
    };
    app_manager_register(&desc);
}
