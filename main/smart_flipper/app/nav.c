#include "nav.h"
#include "app_manager.h"

#include <stdatomic.h>

static atomic_bool nav_in_progress;

static bool nav_try_claim(void)
{
    bool expected = false;
    return atomic_compare_exchange_strong(&nav_in_progress, &expected, true);
}

static void gesture_handler(lv_event_t *e)
{
    (void)e;
    if(atomic_load(&nav_in_progress)) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if(dir == LV_DIR_RIGHT) {
        nav_back();
        lv_indev_wait_release(lv_indev_active());
    }
}

void nav_back(void)
{
    if(!nav_try_claim()) return;

    AppId cur = app_manager_get_current();
    if(cur == APP_ID_NONE) {
        atomic_store(&nav_in_progress, false);
        return;
    }

    const AppDescriptor *desc = app_manager_get_descriptor(cur);
    if(desc && desc->get_scene_manager) {
        SceneManager *sm = desc->get_scene_manager();
        if(sm && !scene_manager_handle_back_event(sm)) {
            app_manager_exit_current();
        }
    } else {
        app_manager_exit_current();
    }

    atomic_store(&nav_in_progress, false);
}

void nav_home(void)
{
    if(!nav_try_claim()) return;
    app_manager_exit_current();
    atomic_store(&nav_in_progress, false);
}

void nav_install_gesture(lv_obj_t *screen)
{
    lv_obj_add_event_cb(screen, gesture_handler, LV_EVENT_GESTURE, NULL);
}
