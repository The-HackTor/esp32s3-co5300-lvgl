#ifndef IR_RUNNER_H
#define IR_RUNNER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct IrRunner IrRunner;

typedef enum {
    IR_RUNNER_EVT_BEGIN = 0,
    IR_RUNNER_EVT_PAUSED,
    IR_RUNNER_EVT_RESUMED,
    IR_RUNNER_EVT_FINISHED,
    IR_RUNNER_EVT_USER_BASE = 0x80,
} IrRunnerEventKind;

typedef bool (*IrRunnerStepFn)(IrRunner *r, void *user_data, size_t step_idx);
typedef void (*IrRunnerEventFn)(void *user_data, uint8_t kind, uint16_t step_idx);

typedef struct {
    size_t           total_steps;
    uint32_t         inter_step_ms;
    IrRunnerStepFn   step_fn;
    IrRunnerEventFn  on_event;
    void            *user_data;
    const char      *task_name;
    uint16_t         stack_words;
    UBaseType_t      priority;
    BaseType_t       core_id;
} IrRunnerConfig;

esp_err_t ir_runner_start(IrRunner **out_r, const IrRunnerConfig *cfg);

void ir_runner_pause(IrRunner *r);
void ir_runner_resume(IrRunner *r);
bool ir_runner_is_paused(const IrRunner *r);
bool ir_runner_finished(const IrRunner *r);
size_t ir_runner_current(const IrRunner *r);
size_t ir_runner_total(const IrRunner *r);

void ir_runner_post_user(IrRunner *r, uint8_t user_kind, uint16_t step_idx);

void ir_runner_destroy(IrRunner **r);

#endif
