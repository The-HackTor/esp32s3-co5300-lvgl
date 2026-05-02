#include "app_manager.h"
#include "ui/arc_menu.h"
#include <string.h>

static struct {
    const AppDescriptor *apps[APP_MAX];
    uint32_t             count;
    AppId                current;
    lv_obj_t            *home_screen;
} mgr;

void app_manager_init(lv_obj_t *home_screen)
{
    memset(&mgr, 0, sizeof(mgr));
    mgr.home_screen = home_screen;
}

void app_manager_register(const AppDescriptor *desc)
{
    if(mgr.count < APP_MAX) {
        mgr.apps[mgr.count++] = desc;
    }
}

void app_manager_init_apps(void)
{
    for(uint32_t i = 0; i < mgr.count; i++) {
        if(mgr.apps[i]->on_init) {
            mgr.apps[i]->on_init();
        }
    }
}

void app_manager_launch(AppId id)
{
    if(mgr.current != APP_ID_NONE) return;

    for(uint32_t i = 0; i < mgr.count; i++) {
        if(mgr.apps[i]->id == id) {
            mgr.current = id;
            arc_menu_hide();

            if(mgr.apps[i]->on_enter) {
                mgr.apps[i]->on_enter();
            } else if(mgr.apps[i]->get_screen) {
                lv_screen_load(mgr.apps[i]->get_screen());
            }
            return;
        }
    }
}

void app_manager_exit_current(void)
{
    if(mgr.current == APP_ID_NONE) return;

    AppId exiting = mgr.current;
    mgr.current = APP_ID_NONE;

    lv_screen_load(mgr.home_screen);

    for(uint32_t i = 0; i < mgr.count; i++) {
        if(mgr.apps[i]->id == exiting) {
            if(mgr.apps[i]->on_leave) {
                mgr.apps[i]->on_leave();
            }
            break;
        }
    }
}

AppId app_manager_get_current(void)
{
    return mgr.current;
}

const AppDescriptor *app_manager_get_descriptor(AppId id)
{
    for(uint32_t i = 0; i < mgr.count; i++) {
        if(mgr.apps[i]->id == id) return mgr.apps[i];
    }
    return NULL;
}

uint32_t app_manager_get_app_count(void)
{
    return mgr.count;
}

const AppDescriptor *app_manager_get_app_by_index(uint32_t index)
{
    if(index < mgr.count) return mgr.apps[index];
    return NULL;
}

lv_obj_t *app_manager_get_home_screen(void)
{
    return mgr.home_screen;
}
