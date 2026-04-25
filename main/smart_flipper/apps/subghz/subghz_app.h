#ifndef SUBGHZ_APP_H
#define SUBGHZ_APP_H

#include <lvgl.h>
#include "app/scene_manager.h"
#include "app/view_dispatcher.h"
#include "ui/modules/view_submenu.h"
#include "ui/modules/view_action.h"
#include "ui/modules/view_info.h"
#include "ui/modules/view_dialog.h"
#include "ui/modules/view_popup.h"
#include "ui/modules/view_custom.h"
#include "hw/hw_types.h"
#include "hw/hw_subghz.h"

typedef enum {
    SubghzViewSubmenu,
    SubghzViewAction,
    SubghzViewInfo,
    SubghzViewDialog,
    SubghzViewPopup,
    SubghzViewCustom,
} SubghzViewId;

typedef struct {
    lv_obj_t       *screen;
    ViewDispatcher *view_dispatcher;
    SceneManager    scene_mgr;

    /* Pre-allocated view modules */
    ViewSubmenu    *submenu;
    ViewAction     *action;
    ViewInfo       *info;
    ViewDialog     *dialog;
    ViewPopup      *popup;
    ViewCustom     *custom;

    /* App state */
    SimSubghzRaw     raw;
    bool             raw_valid;
    SimSubghzDecoded last_decoded;
    bool             decoded_valid;
    uint32_t         frequency;
    uint8_t          preset;
    int              selected_slot;
    bool             hopping;
    float            rssi_threshold;
} SubghzApp;

void       subghz_app_register(void);
SubghzApp *subghz_app_get(void);

#endif
