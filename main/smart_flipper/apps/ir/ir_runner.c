#include "ir_runner.h"

#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include <lvgl.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ir_runner"

#define RUNNER_QUEUE_DEPTH      16
#define RUNNER_DRAIN_INTERVAL_MS 30
#define RUNNER_STOP_POLL_MS     50
#define RUNNER_DESTROY_WAIT_MS  2000

typedef struct {
    uint8_t  kind;
    uint16_t step_idx;
} IrRunnerEvent;

struct IrRunner {
    QueueHandle_t      queue;
    SemaphoreHandle_t  done_sem;
    TaskHandle_t       task;
    lv_timer_t        *drain_timer;

    volatile bool      stop;
    volatile bool      paused;

    size_t             total;
    size_t             cur;
    bool               finished;

    uint32_t           inter_step_ms;

    IrRunnerStepFn     step_fn;
    IrRunnerEventFn    on_event;
    void              *user_data;
};

static void post_evt(IrRunner *r, uint8_t kind, uint16_t step_idx)
{
    IrRunnerEvent e = { .kind = kind, .step_idx = step_idx };
    xQueueSend(r->queue, &e, 0);
}

static bool sleep_interruptible(IrRunner *r, uint32_t total_ms)
{
    uint32_t left = total_ms;
    while(left > 0 && !r->stop) {
        while(r->paused && !r->stop) vTaskDelay(pdMS_TO_TICKS(RUNNER_STOP_POLL_MS));
        if(r->stop) return false;
        uint32_t chunk = left < RUNNER_STOP_POLL_MS ? left : RUNNER_STOP_POLL_MS;
        vTaskDelay(pdMS_TO_TICKS(chunk));
        left -= chunk;
    }
    return !r->stop;
}

static void runner_task_fn(void *arg)
{
    IrRunner *r = arg;

    for(size_t i = 0; i < r->total && !r->stop; i++) {
        while(r->paused && !r->stop) vTaskDelay(pdMS_TO_TICKS(RUNNER_STOP_POLL_MS));
        if(r->stop) break;

        post_evt(r, IR_RUNNER_EVT_BEGIN, (uint16_t)i);

        if(r->step_fn) r->step_fn(r, r->user_data, i);

        if(r->stop) break;
        if(r->inter_step_ms) sleep_interruptible(r, r->inter_step_ms);
    }

    post_evt(r, IR_RUNNER_EVT_FINISHED, (uint16_t)r->total);
    xSemaphoreGive(r->done_sem);
    vTaskDelete(NULL);
}

static void drain_cb(lv_timer_t *t)
{
    IrRunner *r = lv_timer_get_user_data(t);
    if(!r || !r->queue) return;

    IrRunnerEvent e;
    while(xQueueReceive(r->queue, &e, 0) == pdTRUE) {
        switch(e.kind) {
        case IR_RUNNER_EVT_BEGIN:
            r->cur = (size_t)e.step_idx + 1;
            break;
        case IR_RUNNER_EVT_FINISHED:
            r->finished = true;
            r->cur = r->total;
            break;
        default:
            break;
        }
        if(r->on_event) r->on_event(r->user_data, e.kind, e.step_idx);
    }
}

esp_err_t ir_runner_start(IrRunner **out_r, const IrRunnerConfig *cfg)
{
    if(!out_r || !cfg || !cfg->step_fn) return ESP_ERR_INVALID_ARG;

    IrRunner *r = calloc(1, sizeof(IrRunner));
    if(!r) return ESP_ERR_NO_MEM;

    r->total          = cfg->total_steps;
    r->inter_step_ms  = cfg->inter_step_ms;
    r->step_fn        = cfg->step_fn;
    r->on_event       = cfg->on_event;
    r->user_data      = cfg->user_data;
    r->finished       = (cfg->total_steps == 0);

    r->queue    = xQueueCreate(RUNNER_QUEUE_DEPTH, sizeof(IrRunnerEvent));
    r->done_sem = xSemaphoreCreateBinary();
    if(!r->queue || !r->done_sem) {
        if(r->queue)    vQueueDelete(r->queue);
        if(r->done_sem) vSemaphoreDelete(r->done_sem);
        free(r);
        return ESP_ERR_NO_MEM;
    }

    r->drain_timer = lv_timer_create(drain_cb, RUNNER_DRAIN_INTERVAL_MS, r);

    if(r->total == 0) {
        post_evt(r, IR_RUNNER_EVT_FINISHED, 0);
        xSemaphoreGive(r->done_sem);
        *out_r = r;
        return ESP_OK;
    }

    const char  *name  = cfg->task_name   ? cfg->task_name   : "ir_runner";
    uint16_t     stack = cfg->stack_words ? cfg->stack_words : 4096;
    UBaseType_t  prio  = cfg->priority    ? cfg->priority    : (tskIDLE_PRIORITY + 4);
    BaseType_t   core  = cfg->core_id;

    if(xTaskCreatePinnedToCore(runner_task_fn, name, stack, r, prio,
                               &r->task, core) != pdPASS) {
        if(r->drain_timer) lv_timer_delete(r->drain_timer);
        vQueueDelete(r->queue);
        vSemaphoreDelete(r->done_sem);
        free(r);
        return ESP_ERR_NO_MEM;
    }

    *out_r = r;
    return ESP_OK;
}

void ir_runner_pause(IrRunner *r)
{
    if(!r || r->finished) return;
    if(!r->paused) {
        r->paused = true;
        post_evt(r, IR_RUNNER_EVT_PAUSED, (uint16_t)r->cur);
    }
}

void ir_runner_resume(IrRunner *r)
{
    if(!r || r->finished) return;
    if(r->paused) {
        r->paused = false;
        post_evt(r, IR_RUNNER_EVT_RESUMED, (uint16_t)r->cur);
    }
}

bool ir_runner_is_paused(const IrRunner *r) { return r ? r->paused : false; }
bool ir_runner_finished(const IrRunner *r) { return r ? r->finished : true; }
size_t ir_runner_current(const IrRunner *r) { return r ? r->cur : 0; }
size_t ir_runner_total(const IrRunner *r)   { return r ? r->total : 0; }

void ir_runner_post_user(IrRunner *r, uint8_t user_kind, uint16_t step_idx)
{
    if(!r) return;
    post_evt(r, user_kind, step_idx);
}

void ir_runner_destroy(IrRunner **rp)
{
    if(!rp || !*rp) return;
    IrRunner *r = *rp;

    r->stop   = true;
    r->paused = false;

    if(r->drain_timer) {
        lv_timer_delete(r->drain_timer);
        r->drain_timer = NULL;
    }

    if(r->done_sem) {
        xSemaphoreTake(r->done_sem, pdMS_TO_TICKS(RUNNER_DESTROY_WAIT_MS));
        vSemaphoreDelete(r->done_sem);
    }
    if(r->queue) vQueueDelete(r->queue);
    free(r);
    *rp = NULL;
}
