#include "infrared_worker.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "hw/hw_ir.h"
#include "lib/infrared/encoder_decoder/infrared.h"

#include <stdlib.h>
#include <string.h>

#define WORKER_TAG "ir_worker"

#define EVT_RX_RECEIVED          (1U << 0)
#define EVT_RX_TIMEOUT           (1U << 1)
#define EVT_RX_OVERRUN           (1U << 2)
#define EVT_EXIT                 (1U << 3)
#define EVT_ALL                  (EVT_RX_RECEIVED | EVT_RX_TIMEOUT | EVT_RX_OVERRUN | EVT_EXIT)

#define STREAM_ITEM_BYTES        sizeof(LevelDuration)
#define STREAM_ITEMS             128U
#define STREAM_BYTES             (STREAM_ITEMS * STREAM_ITEM_BYTES)

#define WORKER_STACK_BYTES       6144
#define WORKER_TASK_PRIO         (tskIDLE_PRIORITY + 4)
#define WORKER_TASK_CORE         1

typedef struct {
    bool     level;
    uint32_t duration;
} LevelDuration;

struct InfraredWorkerSignal {
    bool     decoded;
    size_t   timings_cnt;
    union {
        InfraredMessage message;
        struct {
            uint32_t timings[INFRARED_WORKER_RAW_TIMINGS_MAX + 1];
            uint32_t frequency;
            float    duty_cycle;
        } raw;
    };
};

struct InfraredWorker {
    TaskHandle_t           task;
    StreamBufferHandle_t   stream;
    EventGroupHandle_t     events;

    InfraredWorkerSignal   signal;
    InfraredDecoderHandler *decoder;
    bool                   decode_enable;
    bool                   running;
    bool                   overrun;

    InfraredWorkerReceivedSignalCallback received_cb;
    void                                 *received_ctx;
};

static void process_edge(InfraredWorker *w, bool level, uint32_t duration)
{
    const InfraredMessage *msg = NULL;
    if(w->decode_enable) {
        msg = infrared_decode(w->decoder, level, duration);
    }
    if(msg) {
        w->signal.message = *msg;
        w->signal.timings_cnt = 0;
        w->signal.decoded = true;
        if(w->received_cb) w->received_cb(w->received_ctx, &w->signal);
        return;
    }

    if(w->signal.timings_cnt == 0 && !level) return;

    if(w->signal.timings_cnt < INFRARED_WORKER_RAW_TIMINGS_MAX) {
        w->signal.raw.timings[w->signal.timings_cnt++] = duration;
    } else {
        w->overrun = true;
        xEventGroupSetBits(w->events, EVT_RX_OVERRUN);
    }
}

static void process_timeout(InfraredWorker *w)
{
    if(w->signal.timings_cnt < 2) {
        w->signal.timings_cnt = 0;
        return;
    }
    const InfraredMessage *msg = infrared_check_decoder_ready(w->decoder);
    if(msg) {
        w->signal.message = *msg;
        w->signal.timings_cnt = 0;
        w->signal.decoded = true;
    } else {
        w->signal.decoded = false;
        w->signal.raw.frequency  = 38000;
        w->signal.raw.duty_cycle = 0.33f;
    }
    if(w->received_cb) w->received_cb(w->received_ctx, &w->signal);
    w->signal.timings_cnt = 0;
}

static void worker_task(void *arg)
{
    InfraredWorker *w = arg;

    while(1) {
        EventBits_t evts = xEventGroupWaitBits(w->events, EVT_ALL,
                                               pdTRUE, pdFALSE, portMAX_DELAY);
        if(evts & EVT_EXIT) break;

        if(evts & EVT_RX_RECEIVED) {
            LevelDuration ld;
            while(xStreamBufferReceive(w->stream, &ld, sizeof(ld), 0) == sizeof(ld)) {
                if(!w->overrun) process_edge(w, ld.level, ld.duration);
            }
        }
        if(evts & EVT_RX_OVERRUN) {
            infrared_reset_decoder(w->decoder);
            w->signal.timings_cnt = 0;
        }
        if(evts & EVT_RX_TIMEOUT) {
            if(!w->overrun) {
                process_timeout(w);
            } else {
                infrared_reset_decoder(w->decoder);
                w->signal.timings_cnt = 0;
                w->overrun = false;
            }
        }
    }

    vTaskDelete(NULL);
}

static void hw_ir_edge_cb(bool level, uint32_t duration_us, bool eof, void *ctx)
{
    InfraredWorker *w = ctx;
    if(eof) {
        xEventGroupSetBits(w->events, EVT_RX_TIMEOUT);
        return;
    }
    LevelDuration ld = { .level = level, .duration = duration_us };
    size_t sent = xStreamBufferSend(w->stream, &ld, sizeof(ld), 0);
    if(sent == sizeof(ld)) {
        xEventGroupSetBits(w->events, EVT_RX_RECEIVED);
    } else {
        xEventGroupSetBits(w->events, EVT_RX_OVERRUN);
    }
}

InfraredWorker *infrared_worker_alloc(void)
{
    InfraredWorker *w = calloc(1, sizeof(*w));
    if(!w) return NULL;
    w->stream  = xStreamBufferCreate(STREAM_BYTES, STREAM_ITEM_BYTES);
    w->events  = xEventGroupCreate();
    w->decoder = infrared_alloc_decoder();
    w->decode_enable = true;
    if(!w->stream || !w->events || !w->decoder) {
        infrared_worker_free(w);
        return NULL;
    }
    return w;
}

void infrared_worker_free(InfraredWorker *w)
{
    if(!w) return;
    if(w->running) infrared_worker_rx_stop(w);
    if(w->decoder) infrared_free_decoder(w->decoder);
    if(w->events)  vEventGroupDelete(w->events);
    if(w->stream)  vStreamBufferDelete(w->stream);
    free(w);
}

void infrared_worker_rx_start(InfraredWorker *w)
{
    if(!w || w->running) return;
    w->signal.timings_cnt = 0;
    w->overrun = false;
    xEventGroupClearBits(w->events, EVT_ALL);
    xStreamBufferReset(w->stream);
    infrared_reset_decoder(w->decoder);

    BaseType_t ok = xTaskCreatePinnedToCore(worker_task, "ir_worker",
                                            WORKER_STACK_BYTES, w,
                                            WORKER_TASK_PRIO, &w->task,
                                            WORKER_TASK_CORE);
    if(ok != pdPASS) {
        ESP_LOGE(WORKER_TAG, "task create failed");
        return;
    }
    w->running = true;
    hw_ir_rx_edge_start(hw_ir_edge_cb, w);
}

void infrared_worker_rx_stop(InfraredWorker *w)
{
    if(!w || !w->running) return;
    hw_ir_rx_edge_stop();
    xEventGroupSetBits(w->events, EVT_EXIT);
    w->running = false;
    vTaskDelay(pdMS_TO_TICKS(20));
    w->task = NULL;
}

void infrared_worker_rx_set_received_signal_callback(
    InfraredWorker *w,
    InfraredWorkerReceivedSignalCallback cb,
    void *context)
{
    if(!w) return;
    w->received_cb  = cb;
    w->received_ctx = context;
}

void infrared_worker_rx_enable_signal_decoding(InfraredWorker *w, bool enable)
{
    if(w) w->decode_enable = enable;
}

bool infrared_worker_signal_is_decoded(const InfraredWorkerSignal *s)
{
    return s ? s->decoded : false;
}

void infrared_worker_get_raw_signal(const InfraredWorkerSignal *s,
                                    const uint32_t **timings,
                                    size_t *timings_cnt)
{
    if(!s || !timings || !timings_cnt) return;
    *timings = s->raw.timings;
    *timings_cnt = s->timings_cnt;
}

const InfraredMessage *infrared_worker_get_decoded_signal(const InfraredWorkerSignal *s)
{
    if(!s || !s->decoded) return NULL;
    return &s->message;
}
