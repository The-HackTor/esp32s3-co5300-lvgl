#include "subghz_app.h"
#include "subghz_scenes.h"
#include "ui/styles.h"
#include "store/signal_store.h"
#include <string.h>

static void build_meta(SubghzApp *app, struct signal_meta *meta)
{
    meta->freq_khz = app->frequency;
    meta->preset = app->preset;
    meta->sample_count = app->raw.count;
    meta->full_count = app->raw.full_count;
    memset(meta->protocol, 0, sizeof(meta->protocol));
    meta->decoded_data = 0;
    meta->decoded_bits = 0;

    if(app->decoded_valid && app->last_decoded.protocol) {
        strncpy(meta->protocol, app->last_decoded.protocol,
                sizeof(meta->protocol) - 1);
        meta->decoded_data = app->last_decoded.data;
        meta->decoded_bits = app->last_decoded.bits;
    }
}

static void slot_cb(void *context, uint32_t index)
{
    SubghzApp *app = context;
    if(index >= SIGNAL_STORE_MAX_SLOTS || !app->raw_valid) return;

    struct signal_meta meta;
    build_meta(app, &meta);

    char name[SIGNAL_NAME_MAX];
    signal_store_auto_name(name, sizeof(name), &meta);

    signal_store_save((uint8_t)index, name, &meta,
                      app->raw.samples, app->raw.count);
    scene_manager_previous_scene(&app->scene_mgr);
}

void subghz_scene_save_on_enter(void *ctx)
{
    SubghzApp *app = ctx;

    view_submenu_reset(app->submenu);
    view_submenu_set_header(app->submenu, "Save Signal", COLOR_BLUE);

    for(int i = 0; i < SIGNAL_STORE_MAX_SLOTS; i++) {
        char label[48];
        if(signal_store_has((uint8_t)i)) {
            char stored_name[SIGNAL_NAME_MAX];
            struct signal_meta meta;
            signal_store_load((uint8_t)i, stored_name, sizeof(stored_name),
                              &meta, NULL, 0);
            lv_snprintf(label, sizeof(label), "%s (overwrite)", stored_name);
            view_submenu_add_item(app->submenu, LV_SYMBOL_EDIT, label,
                                  COLOR_ORANGE, (uint32_t)i, slot_cb, app);
        } else {
            lv_snprintf(label, sizeof(label), "Slot %d (empty)", i + 1);
            view_submenu_add_item(app->submenu, LV_SYMBOL_SAVE, label,
                                  COLOR_BLUE, (uint32_t)i, slot_cb, app);
        }
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, SubghzViewSubmenu);
}

bool subghz_scene_save_on_event(void *ctx, SceneEvent event)
{
    (void)ctx;
    (void)event;
    return false;
}

void subghz_scene_save_on_exit(void *ctx)
{
    SubghzApp *app = ctx;
    view_submenu_reset(app->submenu);
}
