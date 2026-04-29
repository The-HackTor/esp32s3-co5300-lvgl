#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "hw/hw_sleep.h"
#include "lib/infrared/ir_codecs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "macro_run"

#define MACRO_DRAIN_INTERVAL_MS 30
#define MACRO_QUEUE_DEPTH       8
#define MACRO_STOP_POLL_MS      50
#define MACRO_RUNNER_STACK      4096
#define MACRO_RUNNER_PRIO       5
#define MACRO_RUNNER_CORE       1

typedef enum {
    MACRO_EVT_STEP_BEGIN = 0,
    MACRO_EVT_STEP_SKIPPED,
    MACRO_EVT_FINISHED,
} MacroEventKind;

typedef struct {
    uint8_t  kind;
    uint16_t step_idx;
} MacroEvent;

typedef struct {
    QueueHandle_t      queue;
    SemaphoreHandle_t  done_sem;
    TaskHandle_t       task;
    volatile bool      stop;
    size_t             total;
    size_t             cur;
    bool               finished;
} MacroRunner;

static void render(IrApp *app);

static void post_evt(MacroRunner *r, MacroEventKind k, size_t step)
{
    MacroEvent e = { .kind = (uint8_t)k, .step_idx = (uint16_t)step };
    xQueueSend(r->queue, &e, 0);
}

static bool sleep_interruptible(MacroRunner *r, uint32_t total_ms)
{
    uint32_t left = total_ms;
    while(left > 0 && !r->stop) {
        uint32_t chunk = left < MACRO_STOP_POLL_MS ? left : MACRO_STOP_POLL_MS;
        vTaskDelay(pdMS_TO_TICKS(chunk));
        left -= chunk;
    }
    return !r->stop;
}

static const IrButton *find_button(const IrRemote *r, const char *name)
{
    for(size_t i = 0; i < r->button_count; i++) {
        if(strcmp(r->buttons[i].name, name) == 0) return &r->buttons[i];
    }
    return NULL;
}

static void send_button(const IrButton *b)
{
    if(b->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *raw = &b->signal.raw;
        if(!raw->timings || raw->n_timings == 0) return;
        hw_rgb_set(255, 0, 0);
        hw_ir_send_raw(raw->timings, raw->n_timings,
                       raw->freq_hz ? raw->freq_hz : 38000);
        hw_rgb_off();
        return;
    }

    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s",
             b->signal.parsed.protocol);
    msg.address = b->signal.parsed.address;
    msg.command = b->signal.parsed.command;

    uint16_t *enc_t = NULL;
    size_t    enc_n = 0;
    uint32_t  enc_hz = 38000;
    if(ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz) != ESP_OK) {
        ESP_LOGW(TAG, "encode failed for %s", msg.protocol);
        return;
    }
    hw_rgb_set(255, 0, 0);
    hw_ir_send_raw(enc_t, enc_n, enc_hz);
    hw_rgb_off();
    free(enc_t);
}

static void runner_task(void *arg)
{
    IrApp       *app = arg;
    MacroRunner *r   = app->macro_runner;
    const IrMacro *m = &app->current_macro;

    hw_ir_send_repeat_stop();

    for(size_t i = 0; i < m->step_count; i++) {
        if(r->stop) break;
        post_evt(r, MACRO_EVT_STEP_BEGIN, i);

        const IrMacroStep *s = &m->steps[i];

        char path[IR_REMOTE_PATH_MAX];
        if(ir_store_remote_path(path, sizeof(path), s->remote) != ESP_OK) {
            post_evt(r, MACRO_EVT_STEP_SKIPPED, i);
            continue;
        }

        IrRemote rem = {0};
        if(ir_remote_load(&rem, path) != ESP_OK) {
            ESP_LOGW(TAG, "step %u: remote %s missing", (unsigned)i, s->remote);
            post_evt(r, MACRO_EVT_STEP_SKIPPED, i);
            continue;
        }

        const IrButton *btn = find_button(&rem, s->button);
        if(!btn) {
            ESP_LOGW(TAG, "step %u: button %s not in %s",
                     (unsigned)i, s->button, s->remote);
            ir_remote_free(&rem);
            post_evt(r, MACRO_EVT_STEP_SKIPPED, i);
            continue;
        }

        uint8_t reps = s->repeat ? s->repeat : 1;
        for(uint8_t k = 0; k < reps && !r->stop; k++) {
            send_button(btn);
            if(k + 1 < reps) sleep_interruptible(r, 120);
        }
        ir_remote_free(&rem);

        if(s->delay_ms > 0) {
            if(!sleep_interruptible(r, s->delay_ms)) break;
        }
    }

    hw_ir_send_repeat_stop();
    hw_rgb_off();

    post_evt(r, MACRO_EVT_FINISHED, m->step_count);
    xSemaphoreGive(r->done_sem);
    vTaskDelete(NULL);
}

static void drain_cb(lv_timer_t *t)
{
    IrApp       *app = lv_timer_get_user_data(t);
    MacroRunner *r   = app->macro_runner;
    if(!r || !r->queue) return;

    MacroEvent e;
    bool dirty = false;
    while(xQueueReceive(r->queue, &e, 0) == pdTRUE) {
        if(e.kind == MACRO_EVT_FINISHED) {
            r->finished = true;
            r->cur = r->total;
        } else if(e.kind == MACRO_EVT_STEP_BEGIN) {
            r->cur = (size_t)e.step_idx + 1;
        }
        dirty = true;
    }
    if(dirty) render(app);
}

static void stop_clicked(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
}

static void render(IrApp *app)
{
    MacroRunner *r = app->macro_runner;

    view_info_reset(app->info);
    view_info_set_header(app->info,
                         r->finished ? "Done" : app->current_macro.name,
                         r->finished ? COLOR_GREEN : COLOR_YELLOW);

    view_info_add_progress(app->info,
                           r->finished ? COLOR_GREEN : COLOR_YELLOW);
    view_info_set_progress(app->info, r->cur, r->total);

    if(!r->finished && r->cur > 0 && r->cur <= r->total) {
        const IrMacroStep *s = &app->current_macro.steps[r->cur - 1];
        view_info_add_field(app->info, "Remote", s->remote, COLOR_CYAN);
        view_info_add_field(app->info, "Button", s->button, COLOR_CYAN);
        if(s->repeat > 1) {
            char rb[16];
            snprintf(rb, sizeof(rb), "x%u", (unsigned)s->repeat);
            view_info_add_field(app->info, "Repeat", rb, COLOR_YELLOW);
        }
    }

    view_info_add_button(app->info, r->finished ? "Back" : "Stop",
                         r->finished ? COLOR_GREEN : COLOR_RED,
                         stop_clicked, app);
}

void ir_scene_macro_run_on_enter(void *ctx)
{
    IrApp *app = ctx;

    /* Macros chain multiple TX bursts back-to-back. Light-sleep mid-
     * run would silently drop the rest of the sequence. */
    hw_sleep_inhibit(true);

    MacroRunner *r = calloc(1, sizeof(MacroRunner));
    r->total       = app->current_macro.step_count;
    r->cur         = 0;
    r->finished    = (r->total == 0);
    r->queue       = xQueueCreate(MACRO_QUEUE_DEPTH, sizeof(MacroEvent));
    r->done_sem    = xSemaphoreCreateBinary();
    app->macro_runner = r;

    app->macro_drain_timer = lv_timer_create(drain_cb, MACRO_DRAIN_INTERVAL_MS, app);

    render(app);
    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewInfo,
                                            (uint32_t)TransitionSlideLeft, 180);

    if(r->total > 0) {
        xTaskCreatePinnedToCore(runner_task, "ir_macro", MACRO_RUNNER_STACK,
                                app, MACRO_RUNNER_PRIO, &r->task,
                                MACRO_RUNNER_CORE);
    } else {
        xSemaphoreGive(r->done_sem);
    }
}

bool ir_scene_macro_run_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void ir_scene_macro_run_on_exit(void *ctx)
{
    IrApp *app = ctx;
    hw_sleep_inhibit(false);
    MacroRunner *r = app->macro_runner;
    if(!r) return;

    r->stop = true;

    if(app->macro_drain_timer) {
        lv_timer_delete(app->macro_drain_timer);
        app->macro_drain_timer = NULL;
    }

    if(r->done_sem) {
        xSemaphoreTake(r->done_sem, pdMS_TO_TICKS(2000));
    }

    hw_ir_send_repeat_stop();
    hw_rgb_off();

    if(r->queue)    vQueueDelete(r->queue);
    if(r->done_sem) vSemaphoreDelete(r->done_sem);
    free(r);
    app->macro_runner = NULL;

    view_info_reset(app->info);
}
