#ifndef IR_APP_H
#define IR_APP_H

#include <lvgl.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app/scene_manager.h"
#include "app/view_dispatcher.h"
#include "ui/modules/view_submenu.h"
#include "ui/modules/view_action.h"
#include "ui/modules/view_info.h"
#include "ui/modules/view_dialog.h"
#include "ui/modules/view_popup.h"
#include "ui/modules/view_custom.h"
#include "ui/modules/view_text_input.h"
#include "store/ir_store.h"
#include "lib/infrared/ir_codecs.h"

#define IR_RX_TIMINGS_MAX  2048

typedef enum {
    IrViewSubmenu,
    IrViewAction,
    IrViewInfo,
    IrViewDialog,
    IrViewPopup,
    IrViewCustom,
    IrViewTextInput,
} IrViewId;

typedef struct {
    uint16_t timings[IR_RX_TIMINGS_MAX];
    size_t   n_timings;
} IrRxFrame;

typedef struct {
    lv_obj_t       *screen;
    ViewDispatcher *view_dispatcher;
    SceneManager    scene_mgr;

    ViewSubmenu    *submenu;
    ViewAction     *action;
    ViewInfo       *info;
    ViewDialog     *dialog;
    ViewPopup      *popup;
    ViewCustom     *custom;
    ViewTextInput  *text_input;

    IrRemote        current_remote;
    IrButton        pending_button;
    bool            pending_valid;
    bool            is_learning_new_remote;
    char            name_buffer[IR_REMOTE_NAME_MAX];
    int             selected_button_idx;
    int             universal_category;

    QueueHandle_t   rx_queue;
    lv_timer_t     *rx_drain_timer;
    IrDecoded       last_decoded;
    bool            last_decoded_valid;

    /* Raw timing buffer kept alongside a parsed decode so live Send always
     * works even when the encoder doesn't have the protocol yet. Cleared
     * when the pending button is saved (parsed-only goes to .ir) or when
     * the app exits. */
    uint16_t       *pending_raw_timings;
    size_t          pending_raw_n;
} IrApp;

void   ir_app_register(void);
IrApp *ir_app_get(void);

#endif
