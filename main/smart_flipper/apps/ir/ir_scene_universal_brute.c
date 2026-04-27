#include "ir_app.h"
#include "ir_scenes.h"
#include "ir_runner.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/ir_codecs.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ir_univ_brute"

#define BRUTE_GAP_MS         150
#define BRUTE_AC_GAP_MS      250
#define BRUTE_RUNNER_STACK   4096
#define BRUTE_RUNNER_PRIO    5
#define BRUTE_RUNNER_CORE    1

static void render(IrApp *app);

static bool send_signal(const IrButton *btn)
{
    if(!btn) return false;
    if(btn->signal.type == INFRARED_SIGNAL_RAW) {
        const InfraredSignalRaw *r = &btn->signal.raw;
        if(!r->timings || r->n_timings == 0) return false;
        hw_ir_send_raw(r->timings, r->n_timings,
                       r->freq_hz ? r->freq_hz : 38000);
        return true;
    }

    IrDecoded msg = {0};
    snprintf(msg.protocol, sizeof(msg.protocol), "%s",
             btn->signal.parsed.protocol);
    msg.address = btn->signal.parsed.address;
    msg.command = btn->signal.parsed.command;

    uint16_t *enc_t  = NULL;
    size_t    enc_n  = 0;
    uint32_t  enc_hz = 38000;
    if(ir_codecs_encode(&msg, &enc_t, &enc_n, &enc_hz) != ESP_OK) return false;
    if(enc_t && enc_n) hw_ir_send_raw(enc_t, enc_n, enc_hz);
    free(enc_t);
    return true;
}

static bool brute_step(IrRunner *r, void *user_data, size_t step_idx)
{
    (void)r;
    IrApp *app = user_data;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;

    const IrButton *btn = ir_universal_db_button_signal(cat,
                                                        (size_t)app->univ_button_idx,
                                                        step_idx);
    if(!btn) return false;

    hw_rgb_set(255, 0, 0);
    bool ok = send_signal(btn);
    hw_rgb_off();
    return ok;
}

static void brute_evt(void *user_data, uint8_t kind, uint16_t step_idx)
{
    IrApp *app = user_data;
    if(kind == IR_RUNNER_EVT_BEGIN) {
        app->univ_signal_idx = (size_t)step_idx;
    }
    render(app);
}

static void on_stop_or_resume(void *ctx)
{
    IrApp *app = ctx;
    IrRunner *r = app->univ_runner;
    if(!r) return;
    if(ir_runner_finished(r)) {
        scene_manager_previous_scene(&app->scene_mgr);
        return;
    }
    if(ir_runner_is_paused(r)) {
        ir_runner_resume(r);
    } else {
        ir_runner_pause(r);
    }
    render(app);
}

static void on_back(void *ctx)
{
    IrApp *app = ctx;
    scene_manager_previous_scene(&app->scene_mgr);
}

static void on_worked(void *ctx)
{
    IrApp *app = ctx;
    IrRunner *r = app->univ_runner;
    if(!r) return;

    ir_runner_pause(r);

    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    const IrButton *btn = ir_universal_db_button_signal(cat,
                                                        (size_t)app->univ_button_idx,
                                                        app->univ_signal_idx);
    if(!btn) { render(app); return; }

    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }
    if(ir_button_dup(&app->univ_save_button, btn) != ESP_OK) {
        render(app);
        return;
    }
    app->univ_save_valid = true;

    scene_manager_next_scene(&app->scene_mgr, ir_SCENE_UniversalSave);
}

static void render(IrApp *app)
{
    IrRunner *r = app->univ_runner;
    if(!r) return;

    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    size_t total = ir_runner_total(r);
    size_t cur   = ir_runner_current(r);
    bool   done  = ir_runner_finished(r);
    bool   pausd = ir_runner_is_paused(r);

    const char *btn_name = ir_universal_db_button_name(cat,
                                                       (size_t)app->univ_button_idx);

    view_info_reset(app->info);
    view_info_set_header(app->info,
                         btn_name ? btn_name : "Brute",
                         done ? COLOR_GREEN : (pausd ? COLOR_YELLOW : COLOR_RED));

    view_info_add_progress(app->info, done ? COLOR_GREEN : COLOR_RED);
    view_info_set_progress(app->info, cur, total);

    const IrButton *cur_btn = NULL;
    if(!done && cur > 0 && cur <= total) {
        cur_btn = ir_universal_db_button_signal(cat, (size_t)app->univ_button_idx,
                                                cur - 1);
    }

    if(cur_btn) {
        const char *proto = (cur_btn->signal.type == INFRARED_SIGNAL_PARSED)
                                ? cur_btn->signal.parsed.protocol
                                : "RAW";
        view_info_add_field(app->info, "Brand",   cur_btn->name,         COLOR_CYAN);
        view_info_add_field(app->info, "Protocol", proto,                COLOR_PRIMARY);
    } else if(done) {
        view_info_add_field(app->info, "Status", "All sent — none?",     COLOR_DIM);
    }

    if(done) {
        view_info_add_button(app->info, "Back", COLOR_GREEN, on_back, app);
    } else if(pausd) {
        view_info_add_button_row(app->info,
                                 "Resume", COLOR_RED,    on_stop_or_resume, app,
                                 "Save",   COLOR_GREEN,  on_worked,         app);
    } else {
        view_info_add_button_row(app->info,
                                 "Stop",     COLOR_RED,   on_stop_or_resume, app,
                                 "Worked!",  COLOR_GREEN, on_worked,         app);
    }
}

void ir_scene_universal_brute_on_enter(void *ctx)
{
    IrApp *app = ctx;
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    size_t total = ir_universal_db_signal_count(cat, (size_t)app->univ_button_idx);

    app->univ_signal_idx = 0;
    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }

    const IrRunnerConfig cfg = {
        .total_steps    = total,
        .inter_step_ms  = (cat == IR_UNIVERSAL_CAT_AC) ? BRUTE_AC_GAP_MS : BRUTE_GAP_MS,
        .step_fn        = brute_step,
        .on_event       = brute_evt,
        .user_data      = app,
        .task_name      = "ir_brute",
        .stack_words    = BRUTE_RUNNER_STACK,
        .priority       = BRUTE_RUNNER_PRIO,
        .core_id        = BRUTE_RUNNER_CORE,
    };

    IrRunner *r = NULL;
    esp_err_t err = ir_runner_start(&r, &cfg);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "runner start: %s", esp_err_to_name(err));
        scene_manager_previous_scene(&app->scene_mgr);
        return;
    }
    app->univ_runner = r;

    render(app);
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
    if(app->univ_runner) {
        ir_runner_destroy((IrRunner **)&app->univ_runner);
    }
    hw_rgb_off();
    view_info_reset(app->info);
}
