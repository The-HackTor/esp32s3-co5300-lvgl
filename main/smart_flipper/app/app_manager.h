#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <lvgl.h>
#include "scene_manager.h"
#include <stdbool.h>

#define APP_MAX 16

typedef enum {
    APP_ID_NONE = 0,
    APP_ID_NFC,
    APP_ID_SUBGHZ,
    APP_ID_IR,
    APP_ID_HEALTH,
    APP_ID_WEATHER,
    APP_ID_MUSIC,
    APP_ID_NOTIFICATIONS,
    APP_ID_SETTINGS,
    APP_ID_COUNT
} AppId;

/*
 * Persistent-screen lifecycle: on_init allocates once at boot and screens
 * stay resident for instant re-entry; on_enter/on_leave must not alloc/free.
 */
typedef struct {
    AppId         id;
    const char   *name;
    const char   *icon;
    lv_color_t    color;
    void        (*on_init)(void);
    void        (*on_enter)(void);
    void        (*on_leave)(void);
    lv_obj_t   *(*get_screen)(void);
    SceneManager *(*get_scene_manager)(void);
} AppDescriptor;

void                 app_manager_init(lv_obj_t *home_screen);
void                 app_manager_register(const AppDescriptor *desc);
void                 app_manager_init_apps(void);
void                 app_manager_launch(AppId id);
void                 app_manager_exit_current(void);
AppId                app_manager_get_current(void);
const AppDescriptor *app_manager_get_descriptor(AppId id);
uint32_t             app_manager_get_app_count(void);
const AppDescriptor *app_manager_get_app_by_index(uint32_t index);
lv_obj_t            *app_manager_get_home_screen(void);

#endif
