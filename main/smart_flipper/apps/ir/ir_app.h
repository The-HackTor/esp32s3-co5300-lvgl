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
#include "store/macro_store.h"
#include "lib/infrared/ir_codecs.h"
#include "lib/infrared/ac/ac_brand.h"
#include "lib/infrared/worker/infrared_worker.h"

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
    bool            decoded;
    InfraredMessage message;
    size_t          n_timings;
    uint16_t        timings[IR_RX_TIMINGS_MAX];
    uint32_t        frequency;
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
    int             edit_op;
    int             universal_category;
    int             univ_button_idx;
    size_t          univ_signal_idx;
    IrButton        univ_save_button;
    bool            univ_save_valid;
    void           *univ_runner;

    char            remote_action_target[IR_REMOTE_NAME_MAX];

    QueueHandle_t   rx_queue;
    lv_timer_t     *rx_drain_timer;
    InfraredWorker *worker;
    IrDecoded       last_decoded;
    bool            last_decoded_valid;

    /* Raw timings retained alongside a parsed decode so live Send works
     * even when the encoder lacks the protocol. */
    uint16_t       *pending_raw_timings;
    size_t          pending_raw_n;

    uint16_t        preview_timings[256];
    size_t          preview_n;
    uint32_t        preview_seq;
    lv_timer_t     *learn_redraw_timer;

    AcState         ac_state;
    const AcBrand  *ac_brand;
    lv_timer_t     *ac_send_timer;
    bool            ac_dirty;

    IrMacro         current_macro;
    int             selected_step_idx;
    IrMacroStep     pending_step;
    char            macro_name_buf[IR_MACRO_NAME_MAX];
    QueueHandle_t   macro_evt_queue;
    lv_timer_t     *macro_drain_timer;
    void           *macro_runner;
} IrApp;

void   ir_app_register(void);
IrApp *ir_app_get(void);

/* Refcounted pause/resume of the RX subsystem (RMT channel + drain timer
 * + queue). TX-firing scenes use this to suppress self-echo decode and the
 * heap churn of rx_drain. Mirrors Flipper's per-scene worker lifecycle. */
void   ir_app_rx_pause(void);
void   ir_app_rx_resume(void);

void   ir_app_rx_resume_then_delete_timer(lv_timer_t *t);

/* Pre-arm refcount to 1 at boot; first resume (Learn::on_enter) drops it
 * to 0 and starts RX for the first time. */
void   ir_app_rx_pause_seed_initial(void);

#endif
