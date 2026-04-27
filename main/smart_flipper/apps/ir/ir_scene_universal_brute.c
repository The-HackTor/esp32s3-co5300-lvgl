#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "store/ir_settings.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/brute/ir_brute.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ir_univ_brute"

#define BRUTE_TICK_MS         50
#define BRUTE_INTER_FRAME_MS  110
#define BRUTE_CANCEL_WAIT_MS  300
#define BRUTE_EVT_QUEUE_DEPTH 8

typedef struct {
    uint32_t     id;
    HwIrTxResult result;
} BruteEvt;

typedef struct {
    IrBruteContext  bc;
    size_t          cur;          /* index of step currently in flight, or next to dispatch when idle */
    size_t          total;
    bool            paused;
    bool            finished;
    bool            in_flight;
    bool            initial_render_done;

    size_t          rendered_cur;
    bool            rendered_paused;
    bool            rendered_finished;
    bool            rendered_in_flight;

    QueueHandle_t   evt_queue;
    lv_timer_t     *tick;

    uint32_t        last_tx_id;
} BruteState;

static BruteState s_st;

static void render(IrApp *app);
static void render_if_dirty(IrApp *app);
static void brute_tick_cb(lv_timer_t *t);
static void on_tx_done(uint32_t id, HwIrTxResult result, void *ctx);

static void on_stop_or_resume(void *ctx);
static void on_back(void *ctx);
static void on_worked(void *ctx);

static void brute_submit_next(IrApp *app)
{
    if(s_st.cur >= s_st.total) { s_st.finished = true; return; }

    uint16_t *t = NULL;
    size_t    n = 0;
    uint32_t  hz = 38000;
    uint8_t   mr = 1;
    esp_err_t err = ir_brute_step_encode(&s_st.bc, s_st.cur, &t, &n, &hz, &mr);
    if(err != ESP_OK || !t || n == 0) {
        if(t) free(t);
        ESP_LOGW(TAG, "encode step %u: %s", (unsigned)s_st.cur, esp_err_to_name(err));
        s_st.cur += 1;
        if(s_st.cur >= s_st.total) s_st.finished = true;
        return;
    }

    /* Match Flipper: each "send" fires MAX(protocol_min_repeat, user setting).
     * NEC=1, Samsung=1, RC5/6=1; SIRC=3 (Sony); Pioneer=2. RAW + brand
     * encoders use the user setting only (min_repeat = 1). */
    uint8_t user_reps = ir_settings()->brute_repeat;
    if(user_reps == 0) user_reps = 1;
    uint8_t reps = user_reps > mr ? user_reps : mr;

    HwIrTxRequest req = {
        .timings    = t,
        .n_timings  = n,
        .carrier_hz = hz,
        .repeat     = reps,
        .gap_ms     = BRUTE_INTER_FRAME_MS,
        .on_done    = on_tx_done,
        .ctx        = app,
    };
    uint32_t id = hw_ir_tx_submit(&req);
    free(t);
    if(id == 0) {
        ESP_LOGW(TAG, "submit step %u failed", (unsigned)s_st.cur);
        s_st.cur += 1;
        if(s_st.cur >= s_st.total) s_st.finished = true;
        return;
    }
    s_st.last_tx_id = id;
    s_st.in_flight  = true;
    hw_rgb_set(255, 0, 0);
}

static void on_tx_done(uint32_t id, HwIrTxResult result, void *ctx)
{
    (void)ctx;
    BruteEvt e = { .id = id, .result = result };
    if(s_st.evt_queue) xQueueSend(s_st.evt_queue, &e, 0);
}

static void brute_tick_cb(lv_timer_t *t)
{
    IrApp *app = lv_timer_get_user_data(t);
    if(!app || !s_st.evt_queue) return;

    BruteEvt e;
    while(xQueueReceive(s_st.evt_queue, &e, 0) == pdTRUE) {
        if(e.id != s_st.last_tx_id) continue;
        s_st.in_flight = false;
        hw_rgb_off();

        if(e.result == HW_IR_TX_CANCELLED) {
            /* Cancelled by pause / Worked!. Don't advance; the next dispatch
             * will refire the same step on resume. */
            continue;
        }
        s_st.cur += 1;
        if(s_st.cur >= s_st.total) s_st.finished = true;
    }

    if(!s_st.in_flight && !s_st.finished && !s_st.paused) {
        brute_submit_next(app);
    }

    render_if_dirty(app);
}

static void render_if_dirty(IrApp *app)
{
    if(s_st.initial_render_done &&
       s_st.cur        == s_st.rendered_cur &&
       s_st.paused     == s_st.rendered_paused &&
       s_st.finished   == s_st.rendered_finished &&
       s_st.in_flight  == s_st.rendered_in_flight) {
        return;
    }
    render(app);
    s_st.rendered_cur       = s_st.cur;
    s_st.rendered_paused    = s_st.paused;
    s_st.rendered_finished  = s_st.finished;
    s_st.rendered_in_flight = s_st.in_flight;
    s_st.initial_render_done = true;
}

static void render(IrApp *app)
{
    const size_t shown_cur = s_st.cur > s_st.total ? s_st.total : s_st.cur;
    const size_t info_idx  = shown_cur < s_st.total ? shown_cur : (s_st.total ? s_st.total - 1 : 0);

    const char *btn_label = ir_universal_db_button_name(s_st.bc.cat,
                                                        (size_t)s_st.bc.button_idx);

    view_info_reset(app->info);
    view_info_set_header(app->info,
                         btn_label ? btn_label : "Brute",
                         s_st.finished ? COLOR_GREEN
                                       : (s_st.paused ? COLOR_YELLOW : COLOR_RED));

    view_info_add_progress(app->info, s_st.finished ? COLOR_GREEN : COLOR_RED);
    view_info_set_progress(app->info, shown_cur, s_st.total);

    IrBruteStepInfo info = {0};
    if(!s_st.finished && s_st.total > 0 &&
       ir_brute_step_info(&s_st.bc, info_idx, &info)) {
        view_info_add_field(app->info, "Brand",    info.brand_name ? info.brand_name : "?", COLOR_CYAN);
        view_info_add_field(app->info, "Protocol", info.protocol   ? info.protocol   : "?", COLOR_PRIMARY);
    } else if(s_st.finished) {
        view_info_add_field(app->info, "Status", "All sent — none?", COLOR_DIM);
    }

    if(s_st.finished) {
        view_info_add_button(app->info, "Back", COLOR_GREEN, on_back, app);
    } else if(s_st.paused) {
        view_info_add_button_row(app->info,
                                 "Resume", COLOR_RED,    on_stop_or_resume, app,
                                 "Save",   COLOR_GREEN,  on_worked,         app);
    } else {
        view_info_add_button_row(app->info,
                                 "Stop",     COLOR_RED,   on_stop_or_resume, app,
                                 "Worked!",  COLOR_GREEN, on_worked,         app);
    }
}

static void on_stop_or_resume(void *ctx)
{
    IrApp *app = ctx;
    if(s_st.finished) {
        scene_manager_previous_scene(&app->scene_mgr);
        return;
    }
    if(s_st.paused) {
        s_st.paused = false;
        /* The next tick will dispatch the (un-incremented) cur step. */
    } else {
        s_st.paused = true;
        if(s_st.in_flight && s_st.last_tx_id) {
            hw_ir_tx_cancel(s_st.last_tx_id);
        }
    }
    render_if_dirty(app);
}

static void on_back(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
}

static void on_worked(void *ctx)
{
    IrApp *app = ctx;

    /* Freeze the state machine and stop the in-flight burst before
     * re-encoding, so the worker isn't writing to the buffer we're about
     * to copy. Wait briefly for the cancel ack to land in our queue. */
    s_st.paused = true;
    if(s_st.in_flight && s_st.last_tx_id) {
        hw_ir_tx_cancel(s_st.last_tx_id);
    }
    hw_ir_tx_cancel_all(BRUTE_CANCEL_WAIT_MS);
    s_st.in_flight = false;
    hw_rgb_off();

    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }

    /* `cur` is the step IN FLIGHT (next to be confirmed). When pause hit
     * mid-burst, cur is the step the user just heard fire. */
    const size_t pick = (s_st.cur < s_st.total) ? s_st.cur :
                        (s_st.total ? s_st.total - 1 : 0);
    if(ir_brute_step_to_button(&s_st.bc, pick, &app->univ_save_button) != ESP_OK) {
        render_if_dirty(app);
        return;
    }
    app->univ_save_valid = true;
    app->univ_signal_idx = pick;

    if(ir_settings()->auto_save_worked) {
        char auto_name[IR_REMOTE_NAME_MAX];
        IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
        const char *btn_label = ir_universal_db_button_name(cat,
                                                            (size_t)app->univ_button_idx);
        snprintf(auto_name, sizeof(auto_name), "%.16s_%s",
                 app->univ_save_button.name,
                 btn_label ? btn_label : "Found");
        for(char *p = auto_name; *p; p++) {
            if(*p == ' ' || *p == '/' || *p == '\\') *p = '_';
        }

        IrRemote rem = {0};
        if(ir_remote_init(&rem, auto_name) == ESP_OK) {
            IrButton clone = {0};
            if(ir_button_dup(&clone, &app->univ_save_button) == ESP_OK) {
                if(btn_label) snprintf(clone.name, sizeof(clone.name), "%s", btn_label);
                ir_remote_append_button(&rem, &clone);
                ir_button_free(&clone);
            }
            ir_remote_save(&rem);
            ir_remote_free(&rem);

            view_popup_reset(app->popup);
            view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
            view_popup_set_header(app->popup, "Saved", COLOR_GREEN);
            view_popup_set_text(app->popup, auto_name);
            view_popup_set_timeout(app->popup, 1000, NULL, NULL);
            view_dispatcher_switch_to_view_animated(app->view_dispatcher,
                                                    IrViewPopup,
                                                    (uint32_t)TransitionFadeIn, 120);
        }

        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
        scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                          ir_SCENE_UniversalCategory);
        return;
    }

    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_UniversalSave);
}

void ir_scene_universal_brute_on_enter(void *ctx)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;

    memset(&s_st, 0, sizeof(s_st));
    ir_brute_init(&s_st.bc, cat, app->univ_button_idx);
    s_st.total = ir_brute_total(&s_st.bc);
    s_st.finished = (s_st.total == 0);

    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }
    app->univ_signal_idx = 0;

    s_st.evt_queue = xQueueCreate(BRUTE_EVT_QUEUE_DEPTH, sizeof(BruteEvt));
    if(!s_st.evt_queue) {
        ESP_LOGE(TAG, "queue alloc failed");
        scene_manager_previous_scene(&app->scene_mgr);
        return;
    }

    /* No RX during brute: matches Flipper's per-scene worker lifecycle and
     * eliminates self-echo decode pollution + heap churn from rx_drain. */
    ir_app_rx_pause();

    s_st.tick = lv_timer_create(brute_tick_cb, BRUTE_TICK_MS, app);

    ir_brute_log_next_send();
    hw_ir_log_next_send();

    render(app);
    s_st.rendered_cur       = s_st.cur;
    s_st.rendered_paused    = s_st.paused;
    s_st.rendered_finished  = s_st.finished;
    s_st.rendered_in_flight = s_st.in_flight;
    s_st.initial_render_done = true;

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewInfo,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_universal_brute_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_universal_brute_on_exit(void *ctx)
{
    IrApp *app = ctx;

    /* Stop dispatching first so the tick callback can't submit anything new. */
    if(s_st.tick) { lv_timer_delete(s_st.tick); s_st.tick = NULL; }

    /* Cancel everything; bounded wait for the worker to drop its current burst. */
    hw_ir_tx_cancel_all(BRUTE_CANCEL_WAIT_MS);

    /* Now nothing can post into our queue (worker is idle). Drop and free. */
    if(s_st.evt_queue) {
        BruteEvt drain;
        while(xQueueReceive(s_st.evt_queue, &drain, 0) == pdTRUE) {}
        vQueueDelete(s_st.evt_queue);
        s_st.evt_queue = NULL;
    }

    s_st.in_flight = false;
    s_st.last_tx_id = 0;

    /* Restore the IR RX subsystem so subsequent scenes (Learn, History, etc.)
     * resume capturing. Mirror Flipper's per-scene worker lifecycle. */
    ir_app_rx_resume();

    hw_rgb_off();
    view_info_reset(app->info);
}
