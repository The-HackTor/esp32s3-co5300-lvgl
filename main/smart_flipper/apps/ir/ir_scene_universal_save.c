#include "ir_app.h"
#include "ir_scenes.h"
#include "ui/styles.h"
#include "ui/transition.h"
#include "store/ir_store.h"
#include "lib/infrared/universal_db/ir_universal_db.h"
#include "esp_log.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TAG "ir_univ_save"

static void sanitize_name(char *s)
{
    for(char *p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if(c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
           c == '"' || c == '<' || c == '>' || c == '|' || c < 0x20) {
            *p = '_';
        }
    }
}

static void name_accepted(void *ctx, const char *text)
{
    IrApp *app = ctx;
    strncpy(app->name_buffer, text, sizeof(app->name_buffer) - 1);
    app->name_buffer[sizeof(app->name_buffer) - 1] = '\0';
    scene_manager_handle_custom_event(&app->scene_mgr, IR_EVT_UNIV_SAVE_NAME_ACCEPTED);
}

static esp_err_t do_save(IrApp *app)
{
    if(!app->univ_save_valid) return ESP_ERR_INVALID_STATE;

    sanitize_name(app->name_buffer);
    if(app->name_buffer[0] == '\0') return ESP_ERR_INVALID_ARG;

    IrRemote r = {0};
    esp_err_t err = ir_remote_init(&r, app->name_buffer);
    if(err != ESP_OK) return err;

    /* Store under the universal-DB button label (e.g. "POWER") so the user
     * sees the action they were trying to find when reopening the remote. */
    IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
    const char *btn_label = ir_universal_db_button_name(cat,
                                                        (size_t)app->univ_button_idx);
    IrButton b = {0};
    err = ir_button_dup(&b, &app->univ_save_button);
    if(err != ESP_OK) { ir_remote_free(&r); return err; }
    if(btn_label) {
        snprintf(b.name, sizeof(b.name), "%s", btn_label);
    }

    err = ir_remote_append_button(&r, &b);
    if(err != ESP_OK) {
        ir_button_free(&b);
        ir_remote_free(&r);
        return err;
    }
    /* append_button copies; free our local */
    ir_button_free(&b);

    err = ir_remote_save(&r);
    ir_remote_free(&r);
    if(err == ESP_OK) {
        if(app->univ_save_button.signal.type == INFRARED_SIGNAL_PARSED) {
            ir_recents_append(app->univ_save_button.name,
                              app->univ_save_button.signal.parsed.protocol,
                              app->univ_save_button.signal.parsed.address,
                              app->univ_save_button.signal.parsed.command);
        } else {
            ir_recents_append(app->univ_save_button.name, "RAW", 0, 0);
        }
    }
    return err;
}

void ir_scene_universal_save_on_enter(void *ctx)
{
    IrApp *app = ctx;

    view_text_input_reset(app->text_input);
    view_text_input_set_header(app->text_input, "Save Remote", COLOR_GREEN);

    if(app->univ_save_valid) {
        IrUniversalCategory cat = (IrUniversalCategory)app->universal_category;
        const char *brand = app->univ_save_button.name;
        const char *btn_label = ir_universal_db_button_name(cat,
                                                            (size_t)app->univ_button_idx);
        if(brand && brand[0] && btn_label) {
            snprintf(app->name_buffer, sizeof(app->name_buffer), "%.20s_%s",
                     brand, btn_label);
        } else if(btn_label) {
            snprintf(app->name_buffer, sizeof(app->name_buffer), "Universal_%s",
                     btn_label);
        } else {
            snprintf(app->name_buffer, sizeof(app->name_buffer), "Found");
        }
    } else {
        app->name_buffer[0] = '\0';
    }
    sanitize_name(app->name_buffer);

    view_text_input_set_buffer(app->text_input, app->name_buffer,
                               sizeof(app->name_buffer));
    view_text_input_set_callback(app->text_input, name_accepted, app);

    view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewTextInput,
                                            (uint32_t)TransitionSlideLeft, 180);
}

bool ir_scene_universal_save_on_event(void *ctx, SceneEvent event)
{
    IrApp *app = ctx;
    if(event.type == SceneEventTypeCustom && event.event == IR_EVT_UNIV_SAVE_NAME_ACCEPTED) {
        esp_err_t err = do_save(app);

        view_popup_reset(app->popup);
        if(err == ESP_OK) {
            view_popup_set_icon(app->popup, LV_SYMBOL_OK, COLOR_GREEN);
            view_popup_set_header(app->popup, "Saved", COLOR_GREEN);
            view_popup_set_text(app->popup, app->name_buffer);
        } else {
            ESP_LOGE(TAG, "save: %s", esp_err_to_name(err));
            view_popup_set_icon(app->popup, LV_SYMBOL_WARNING, COLOR_RED);
            view_popup_set_header(app->popup, "Save failed", COLOR_RED);
            view_popup_set_text(app->popup, esp_err_to_name(err));
        }
        view_popup_set_timeout(app->popup, 1200, NULL, NULL);
        view_dispatcher_switch_to_view_animated(app->view_dispatcher, IrViewPopup,
                                                (uint32_t)TransitionFadeIn, 120);

        if(app->univ_save_valid) {
            ir_button_free(&app->univ_save_button);
            app->univ_save_valid = false;
        }

        /* Pop UniversalSave + UniversalBrute, returning to the button list. */
        scene_manager_search_and_switch_to_previous_scene(&app->scene_mgr,
                                                          ir_SCENE_UniversalCategory);
        return true;
    }
    return false;
}

void ir_scene_universal_save_on_exit(void *ctx)
{
    IrApp *app = ctx;
    view_text_input_reset(app->text_input);
}
