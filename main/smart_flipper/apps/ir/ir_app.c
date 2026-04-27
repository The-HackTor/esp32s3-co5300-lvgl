#include "ir_app.h"
#include "ir_scenes.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/styles.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "store/ir_store.h"
#include "store/ir_settings.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/universal_db/ir_universal_index.h"

#include "esp_timer.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#define TAG "ir_app"

#define RX_DRAIN_INTERVAL_MS 30
#define RX_QUEUE_DEPTH 2

static IrApp app;

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_enter,
static const SceneOnEnterCb ir_on_enter[] = {
#include "ir_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_event,
static const SceneOnEventCb ir_on_event[] = {
#include "ir_scene_config.h"
};
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    [prefix##_SCENE_##id] = prefix##_scene_##name##_on_exit,
static const SceneOnExitCb ir_on_exit[] = {
#include "ir_scene_config.h"
};
#undef ADD_SCENE

const SceneManagerHandlers ir_scene_handlers = {
    .on_enter  = ir_on_enter,
    .on_event  = ir_on_event,
    .on_exit   = ir_on_exit,
    .scene_num = IR_SCENE_COUNT,
};

static void rx_worker_cb(const uint16_t *timings, size_t n_timings, void *ctx)
{
    (void)ctx;
    if(!app.rx_queue) return;

    IrRxFrame *frame = malloc(sizeof(IrRxFrame));
    if(!frame) return;

    size_t copy_n = n_timings;
    if(copy_n > IR_RX_TIMINGS_MAX) copy_n = IR_RX_TIMINGS_MAX;
    memcpy(frame->timings, timings, copy_n * sizeof(uint16_t));
    frame->n_timings = copy_n;

    if(xQueueSend(app.rx_queue, &frame, 0) != pdTRUE) {
        free(frame);
    }
}

static void rx_drain_timer_cb(lv_timer_t *t)
{
    (void)t;
    if(!app.rx_queue) return;

    IrRxFrame *frame = NULL;
    if(xQueueReceive(app.rx_queue, &frame, 0) != pdTRUE) return;
    if(!frame) return;

    if(hw_ir_tx_was_recent_us((int64_t)ir_settings()->tx_echo_ms * 1000)) {
        free(frame);
        return;
    }

    hw_rgb_pulse(0, 255, 0, 100);

    size_t prev_n = frame->n_timings < 256 ? frame->n_timings : 256;
    memcpy(app.preview_timings, frame->timings, prev_n * sizeof(uint16_t));
    app.preview_n = prev_n;
    app.preview_seq++;

    IrDecoded dec = {0};
    bool decoded = ir_codecs_decode(frame->timings, frame->n_timings, &dec);

    if(decoded) {
        app.last_decoded = dec;
        app.last_decoded_valid = true;
        ir_button_free(&app.pending_button);
        memset(&app.pending_button, 0, sizeof(app.pending_button));

        IrHistoryEntry hist = {
            .timestamp_us = esp_timer_get_time(),
            .address      = dec.address,
            .command      = dec.command,
        };
        snprintf(hist.protocol, sizeof(hist.protocol), "%s", dec.protocol);
        ir_history_append(&hist);

        if(dec.source == IR_DECODED_FLIPPER) {
            app.pending_button.signal.type = INFRARED_SIGNAL_PARSED;
            snprintf(app.pending_button.signal.parsed.protocol,
                     sizeof(app.pending_button.signal.parsed.protocol),
                     "%s", dec.protocol);
            app.pending_button.signal.parsed.address = dec.address;
            app.pending_button.signal.parsed.command = dec.command;
        } else {
            app.pending_button.signal.type = INFRARED_SIGNAL_RAW;
            app.pending_button.signal.raw.freq_hz  = 38000;
            app.pending_button.signal.raw.duty_pct = 33;
            app.pending_button.signal.raw.timings = malloc(frame->n_timings * sizeof(uint16_t));
            if(app.pending_button.signal.raw.timings) {
                memcpy(app.pending_button.signal.raw.timings, frame->timings,
                       frame->n_timings * sizeof(uint16_t));
                app.pending_button.signal.raw.n_timings = frame->n_timings;
            }
        }
        app.pending_valid = true;

        if(app.pending_raw_timings) free(app.pending_raw_timings);
        app.pending_raw_timings = malloc(frame->n_timings * sizeof(uint16_t));
        if(app.pending_raw_timings) {
            memcpy(app.pending_raw_timings, frame->timings,
                   frame->n_timings * sizeof(uint16_t));
            app.pending_raw_n = frame->n_timings;
        } else {
            app.pending_raw_n = 0;
        }

        scene_manager_handle_custom_event(&app.scene_mgr, IR_EVT_RX_DECODED);
    } else {
        app.last_decoded_valid = false;
        ir_button_free(&app.pending_button);
        memset(&app.pending_button, 0, sizeof(app.pending_button));
        app.pending_button.signal.type = INFRARED_SIGNAL_RAW;
        app.pending_button.signal.raw.freq_hz = 38000;
        app.pending_button.signal.raw.duty_pct = 33;
        app.pending_button.signal.raw.timings = malloc(frame->n_timings * sizeof(uint16_t));
        if(app.pending_button.signal.raw.timings) {
            memcpy(app.pending_button.signal.raw.timings, frame->timings,
                   frame->n_timings * sizeof(uint16_t));
            app.pending_button.signal.raw.n_timings = frame->n_timings;
            app.pending_valid = true;
            scene_manager_handle_custom_event(&app.scene_mgr, IR_EVT_RX_RAW);
        }
    }

    free(frame);
}

static void on_init(void)
{
    ESP_LOGI(TAG, "on_init");
    memset(&app, 0, sizeof(app));

    app.screen = lv_obj_create(NULL);
    lv_obj_add_style(app.screen, &style_screen, 0);
    nav_install_gesture(app.screen);

    app.submenu    = view_submenu_alloc(app.screen);
    app.action     = view_action_alloc(app.screen);
    app.info       = view_info_alloc(app.screen);
    app.dialog     = view_dialog_alloc(app.screen);
    app.popup      = view_popup_alloc(app.screen);
    app.custom     = view_custom_alloc(app.screen);
    app.text_input = view_text_input_alloc(app.screen);

    app.view_dispatcher = view_dispatcher_alloc(app.screen);
    view_dispatcher_add_view(app.view_dispatcher, IrViewSubmenu,   view_submenu_get_module(app.submenu));
    view_dispatcher_add_view(app.view_dispatcher, IrViewAction,    view_action_get_module(app.action));
    view_dispatcher_add_view(app.view_dispatcher, IrViewInfo,      view_info_get_module(app.info));
    view_dispatcher_add_view(app.view_dispatcher, IrViewDialog,    view_dialog_get_module(app.dialog));
    view_dispatcher_add_view(app.view_dispatcher, IrViewPopup,     view_popup_get_module(app.popup));
    view_dispatcher_add_view(app.view_dispatcher, IrViewCustom,    view_custom_get_module(app.custom));
    view_dispatcher_add_view(app.view_dispatcher, IrViewTextInput, view_text_input_get_module(app.text_input));

    view_dispatcher_set_scene_manager(app.view_dispatcher, &app.scene_mgr);

    app.rx_queue = xQueueCreate(RX_QUEUE_DEPTH, sizeof(IrRxFrame *));
    ir_store_init();
    macro_store_init();
    ir_settings_load();
    ir_universal_db_init();
    ir_universal_index_init();
}

static void on_enter(void)
{
    ESP_LOGI(TAG, "on_enter");
    app.is_learning_new_remote = false;
    app.pending_valid = false;
    app.last_decoded_valid = false;
    app.selected_button_idx = -1;
    memset(app.name_buffer, 0, sizeof(app.name_buffer));

    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);
    view_text_input_reset(app.text_input);

    if(app.rx_queue) xQueueReset(app.rx_queue);
    hw_ir_rx_start(rx_worker_cb, NULL);

    if(!app.rx_drain_timer) {
        app.rx_drain_timer = lv_timer_create(rx_drain_timer_cb, RX_DRAIN_INTERVAL_MS, NULL);
    } else {
        lv_timer_resume(app.rx_drain_timer);
    }

    scene_manager_init(&app.scene_mgr, &ir_scene_handlers, &app);
    scene_manager_next_scene(&app.scene_mgr, ir_SCENE_Start);
    lv_screen_load(app.screen);
}

static void on_leave(void)
{
    ESP_LOGI(TAG, "on_leave");
    scene_manager_stop(&app.scene_mgr);

    if(app.rx_drain_timer) lv_timer_pause(app.rx_drain_timer);
    hw_ir_rx_stop();
    if(app.rx_queue) {
        IrRxFrame *frame = NULL;
        while(xQueueReceive(app.rx_queue, &frame, 0) == pdTRUE) {
            if(frame) free(frame);
        }
    }
    hw_rgb_off();

    if(app.pending_valid) {
        ir_button_free(&app.pending_button);
        app.pending_valid = false;
    }
    if(app.pending_raw_timings) {
        free(app.pending_raw_timings);
        app.pending_raw_timings = NULL;
        app.pending_raw_n = 0;
    }
    ir_remote_free(&app.current_remote);
    macro_free(&app.current_macro);

    view_submenu_reset(app.submenu);
    view_action_reset(app.action);
    view_info_reset(app.info);
    view_dialog_reset(app.dialog);
    view_popup_reset(app.popup);
    view_custom_clean(app.custom);
    view_text_input_reset(app.text_input);
}

static lv_obj_t *get_screen(void) { return app.screen; }
static SceneManager *get_scene_manager(void) { return &app.scene_mgr; }

IrApp *ir_app_get(void) { return &app; }

void ir_app_register(void)
{
    static const AppDescriptor desc = {
        .id                = APP_ID_IR,
        .name              = "IR Remote",
        .icon              = LV_SYMBOL_EYE_OPEN,
        .color             = {.blue = 0x35, .green = 0x6B, .red = 0xFF},
        .on_init           = on_init,
        .on_enter          = on_enter,
        .on_leave          = on_leave,
        .get_screen        = get_screen,
        .get_scene_manager = get_scene_manager,
    };
    app_manager_register(&desc);
}
