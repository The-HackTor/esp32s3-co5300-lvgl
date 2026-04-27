#include "ir_app.h"
#include "ir_scenes.h"
#include "ir_runner.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "store/ir_settings.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "lib/infrared/brute/ir_brute.h"
#include "hw/hw_ir.h"
#include "hw/hw_rgb.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ir_univ_brute"

#define BRUTE_GAP_MS         150
#define BRUTE_AC_GAP_MS      250
#define BRUTE_RUNNER_STACK   6144
#define BRUTE_RUNNER_PRIO    5
#define BRUTE_RUNNER_CORE    1

static IrBruteContext s_bc;

static void render(IrApp *app);

static bool brute_step(IrRunner *r, void *user_data, size_t step_idx)
{
    (void)r;
    IrApp *app = user_data;
    hw_rgb_set(255, 0, 0);
    esp_err_t err = ir_brute_step_send(&s_bc, step_idx);
    hw_rgb_off();
    if(err != ESP_OK) {
        ESP_LOGW(TAG, "step %u: %s", (unsigned)step_idx, esp_err_to_name(err));
    }
    (void)app;
    return err == ESP_OK;
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

    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }

    if(ir_brute_step_to_button(&s_bc, app->univ_signal_idx,
                               &app->univ_save_button) != ESP_OK) {
        render(app);
        return;
    }
    app->univ_save_valid = true;

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

static void render(IrApp *app)
{
    IrRunner *r = app->univ_runner;
    if(!r) return;

    size_t total = ir_runner_total(r);
    size_t cur   = ir_runner_current(r);
    bool   done  = ir_runner_finished(r);
    bool   pausd = ir_runner_is_paused(r);

    const char *btn_label = ir_universal_db_button_name(s_bc.cat,
                                                        (size_t)s_bc.button_idx);

    view_info_reset(app->info);
    view_info_set_header(app->info,
                         btn_label ? btn_label : "Brute",
                         done ? COLOR_GREEN : (pausd ? COLOR_YELLOW : COLOR_RED));

    view_info_add_progress(app->info, done ? COLOR_GREEN : COLOR_RED);
    view_info_set_progress(app->info, cur, total);

    IrBruteStepInfo info = {0};
    if(!done && cur > 0 && cur <= total &&
       ir_brute_step_info(&s_bc, cur - 1, &info)) {
        view_info_add_field(app->info, "Brand",    info.brand_name ? info.brand_name : "?", COLOR_CYAN);
        view_info_add_field(app->info, "Protocol", info.protocol   ? info.protocol   : "?", COLOR_PRIMARY);
    } else if(done) {
        view_info_add_field(app->info, "Status", "All sent — none?", COLOR_DIM);
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

    ir_brute_init(&s_bc, cat, app->univ_button_idx);
    size_t total = ir_brute_total(&s_bc);

    app->univ_signal_idx = 0;
    if(app->univ_save_valid) {
        ir_button_free(&app->univ_save_button);
        app->univ_save_valid = false;
    }

    const IrSettings *st = ir_settings();
    const IrRunnerConfig cfg = {
        .total_steps    = total,
        .inter_step_ms  = (cat == IR_UNIVERSAL_CAT_AC) ? st->brute_ac_gap_ms
                                                       : st->brute_gap_ms,
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
