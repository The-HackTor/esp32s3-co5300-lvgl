#ifndef INFRARED_WORKER_H
#define INFRARED_WORKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/infrared/encoder_decoder/infrared.h"

#define INFRARED_WORKER_RAW_TIMINGS_MAX 1024U

typedef struct InfraredWorker InfraredWorker;
typedef struct InfraredWorkerSignal InfraredWorkerSignal;

typedef void (*InfraredWorkerReceivedSignalCallback)(
    void *context, InfraredWorkerSignal *signal);

InfraredWorker *infrared_worker_alloc(void);
void infrared_worker_free(InfraredWorker *w);

void infrared_worker_rx_start(InfraredWorker *w);
void infrared_worker_rx_stop(InfraredWorker *w);

void infrared_worker_rx_set_received_signal_callback(
    InfraredWorker *w,
    InfraredWorkerReceivedSignalCallback cb,
    void *context);

void infrared_worker_rx_enable_signal_decoding(InfraredWorker *w, bool enable);

bool infrared_worker_signal_is_decoded(const InfraredWorkerSignal *s);

void infrared_worker_get_raw_signal(
    const InfraredWorkerSignal *s,
    const uint32_t **timings,
    size_t *timings_cnt);

const InfraredMessage *infrared_worker_get_decoded_signal(const InfraredWorkerSignal *s);

#endif
