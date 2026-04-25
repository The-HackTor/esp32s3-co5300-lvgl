#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "store/signal_store.h"
#include <string.h>

static const char *preset_names[] = {
    "OOK 270kHz", "OOK 650kHz", "2FSK 2.38kHz", "2FSK 47.6kHz",
    "MSK 99.97kBd", "GFSK 9.99kBd", "Packet"
};

enum {
    SignalDetailEvtReplay,
    SignalDetailEvtWaveform,
    SignalDetailEvtDelete,
};

static void replay_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SignalDetailEvtReplay);
}

static void waveform_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SignalDetailEvtWaveform);
}

static void delete_cb(void *ctx)
{
    SubghzApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, SignalDetailEvtDelete);
}

void subghz_scene_signal_detail_on_enter(void *ctx)
{
    SubghzApp *app = ctx;
    int slot = app->selected_slot;

    char name[SIGNAL_NAME_MAX];
    struct signal_meta meta;
    signal_store_load((uint8_t)slot, name, sizeof(name), &meta, NULL, 0);

    view_info_reset(app->info);
    view_info_set_header(app->info, name, COLOR_CYAN);

    uint32_t mhz = meta.freq_khz / 1000;
    uint32_t khz = meta.freq_khz % 1000;
    char freq_buf[32];
    lv_snprintf(freq_buf, sizeof(freq_buf), "%lu.%03lu MHz",
                (unsigned long)mhz, (unsigned long)khz);
    view_info_add_field(app->info, "Frequency", freq_buf, COLOR_YELLOW);

    uint8_t p = (uint8_t)meta.preset;
    view_info_add_field(app->info, "Modulation",
                        preset_names[p < 7 ? p : 0], COLOR_SECONDARY);

    char samples_buf[32];
    lv_snprintf(samples_buf, sizeof(samples_buf), "%u",
                (unsigned)meta.sample_count);
    view_info_add_field(app->info, "Samples", samples_buf, COLOR_SECONDARY);

    if(meta.protocol[0] != '\0') {
        view_info_add_separator(app->info);
        view_info_add_field(app->info, "Protocol", meta.protocol, COLOR_GREEN);

        char data_buf[32];
        lv_snprintf(data_buf, sizeof(data_buf), "0x%08lX",
                    (unsigned long)(meta.decoded_data & 0xFFFFFFFF));
        view_info_add_field(app->info, "Data", data_buf, COLOR_PRIMARY);

        char bits_buf[16];
        lv_snprintf(bits_buf, sizeof(bits_buf), "%u", (unsigned)meta.decoded_bits);
        view_info_add_field(app->info, "Bits", bits_buf, COLOR_SECONDARY);
    }

    view_info_add_separator(app->info);

    view_info_add_button(app->info, LV_SYMBOL_UPLOAD " Replay",
                         COLOR_ORANGE, replay_cb, app);
    view_info_add_button(app->info, LV_SYMBOL_EYE_OPEN " Waveform",
                         COLOR_GREEN, waveform_cb, app);
    view_info_add_button(app->info, LV_SYMBOL_TRASH " Delete",
                         COLOR_RED, delete_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewInfo);
}

bool subghz_scene_signal_detail_on_event(void *ctx, SceneEvent event)
{
    SubghzApp *app = ctx;

    if(event.type == SceneEventTypeCustom) {
        switch(event.event) {
            case SignalDetailEvtReplay:
                scene_manager_next_scene(&app->scene_mgr, subghz_SCENE_Replay);
                return true;
            case SignalDetailEvtWaveform: {
                int slot = app->selected_slot;
                struct signal_meta meta;
                int ret = signal_store_load((uint8_t)slot, NULL, 0, &meta,
                                            app->raw.samples,
                                            SUBGHZ_RAW_MAX_SAMPLES);
                if(ret == 0) {
                    app->raw.count = meta.sample_count;
                    app->raw_valid = true;
                    app->frequency = meta.freq_khz;
                    scene_manager_next_scene(&app->scene_mgr,
                                             subghz_SCENE_Waveform);
                }
                return true;
            }
            case SignalDetailEvtDelete:
                signal_store_delete((uint8_t)app->selected_slot);
                scene_manager_search_and_switch_to_previous_scene(
                    &app->scene_mgr, subghz_SCENE_SavedList);
                return true;
        }
    }
    return false;
}

void subghz_scene_signal_detail_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_info_reset(app->info);
}
