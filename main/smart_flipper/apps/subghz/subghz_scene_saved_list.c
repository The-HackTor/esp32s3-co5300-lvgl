#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "store/signal_store.h"
#include <string.h>

static void card_selected(void *context, uint32_t index)
{
    SubghzApp *app = context;

    if(index >= SIGNAL_STORE_MAX_SLOTS || !signal_store_has((uint8_t)index))
        return;

    char name[SIGNAL_NAME_MAX];
    struct signal_meta meta;
    signal_store_load((uint8_t)index, name, sizeof(name),
                      &meta, app->raw.samples, SIM_SUBGHZ_MAX_SAMPLES);
    app->raw.count = meta.sample_count;
    app->raw.full_count = meta.full_count;
    signal_store_slot_path((uint8_t)index,
                           app->raw.file_path, sizeof(app->raw.file_path));
    app->raw_valid = true;
    app->frequency = meta.freq_khz;
    app->preset = (uint8_t)meta.preset;
    app->selected_slot = (int)index;

    if(meta.protocol[0] != '\0') {
        app->last_decoded.protocol = meta.protocol;
        app->last_decoded.data = meta.decoded_data;
        app->last_decoded.bits = meta.decoded_bits;
        app->decoded_valid = true;
    } else {
        app->decoded_valid = false;
    }

    scene_manager_set_scene_state(&app->scene_mgr, subghz_SCENE_SavedList, index);
    scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_SignalDetail);
}

void subghz_scene_saved_list_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    bool any = false;

    view_submenu_reset(app->submenu);

    int total = signal_store_count();
    char title[32];
    lv_snprintf(title, sizeof(title), "Saved (%d)", total);
    view_submenu_set_header(app->submenu, title, COLOR_BLUE);

    for(int i = 0; i < SIGNAL_STORE_MAX_SLOTS; i++) {
        if(!signal_store_has((uint8_t)i)) continue;
        any = true;

        char name[SIGNAL_NAME_MAX];
        struct signal_meta meta;
        signal_store_load((uint8_t)i, name, sizeof(name), &meta, NULL, 0);

        uint32_t mhz = meta.freq_khz / 1000;
        uint32_t khz = meta.freq_khz % 1000;

        char label[64];
        if(meta.protocol[0] != '\0') {
            lv_snprintf(label, sizeof(label), "%s  %s  %lu.%03lu",
                        name, meta.protocol,
                        (unsigned long)mhz, (unsigned long)khz);
        } else {
            lv_snprintf(label, sizeof(label), "%s  %lu.%03lu MHz",
                        name, (unsigned long)mhz, (unsigned long)khz);
        }

        lv_color_t color = (meta.protocol[0] != '\0') ? COLOR_CYAN : COLOR_SECONDARY;
        view_submenu_add_item(app->submenu, LV_SYMBOL_AUDIO, label,
                              color, (uint32_t)i, card_selected, app);
    }

    if(!any) {
        /* Show empty-state popup instead */
        view_popup_reset(app->popup);
        view_popup_set_icon(app->popup, LV_SYMBOL_AUDIO, COLOR_DIM);
        view_popup_set_header(app->popup, "No Saved Signals", COLOR_SECONDARY);
        view_popup_set_text(app->popup,
                            "Capture a signal first, then save it here.");
        view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewPopup);
        return;
    }

    view_submenu_set_selected_item(app->submenu,
        scene_manager_get_scene_state(&app->scene_mgr, subghz_SCENE_SavedList));

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_saved_list_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_saved_list_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
    view_popup_reset(app->popup);
}
