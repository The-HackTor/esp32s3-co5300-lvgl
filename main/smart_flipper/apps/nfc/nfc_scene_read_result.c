#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

/*
 * Read Result -- card summary with Retry | More.
 * Uses view_info with short labels to avoid circular edge clipping.
 */

enum { NFC_EVT_RETRY = 0x300, NFC_EVT_MORE };

static void retry_cb(void *ctx) {
    NfcApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_RETRY);
}

static void more_cb(void *ctx) {
    NfcApp *app = ctx;
    scene_manager_handle_custom_event(&app->scene_mgr, NFC_EVT_MORE);
}

static void show_mfc_result(NfcApp *app) {
    char buf[48];

    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X",
             app->dump.card.uid[0], app->dump.card.uid[1],
             app->dump.card.uid[2], app->dump.card.uid[3]);
    view_info_add_field(app->info, "UID", buf, COLOR_PRIMARY);

    snprintf(buf, sizeof(buf), "%s (0x%02X)", mfc_type_str(app->dump.type),
             app->dump.card.sak);
    view_info_add_field(app->info, "Type", buf, COLOR_CYAN);

    int total_sectors = mfc_total_sectors(app->dump.type);
    int keys_found = mfc_keys_found(&app->dump);
    int sectors_read = mfc_sectors_read(&app->dump);

    snprintf(buf, sizeof(buf), "%d/%d", keys_found, total_sectors * 2);
    view_info_add_field(app->info, "Keys Found", buf, COLOR_GREEN);

    snprintf(buf, sizeof(buf), "%d/%d", sectors_read, total_sectors);
    view_info_add_field(app->info, "Sectors Read", buf, COLOR_GREEN);

    snprintf(buf, sizeof(buf), "%02X %02X",
             app->dump.card.atqa[0], app->dump.card.atqa[1]);
    view_info_add_field(app->info, "ATQA", buf, COLOR_SECONDARY);
}

static void show_mfu_result(NfcApp *app) {
    char buf[48];

    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X",
             app->mfu_dump.card.uid[0], app->mfu_dump.card.uid[1],
             app->mfu_dump.card.uid[2], app->mfu_dump.card.uid[3],
             app->mfu_dump.card.uid[4], app->mfu_dump.card.uid[5],
             app->mfu_dump.card.uid[6]);
    view_info_add_field(app->info, "UID", buf, COLOR_PRIMARY);

    snprintf(buf, sizeof(buf), "%s", mfu_type_str(app->mfu_dump.type));
    view_info_add_field(app->info, "Type", buf, COLOR_CYAN);

    uint16_t pages_read = 0;
    for (uint16_t p = 0; p < app->mfu_dump.total_pages; p++) {
        if (app->mfu_dump.page_read[p]) pages_read++;
    }
    snprintf(buf, sizeof(buf), "%u/%u", pages_read, app->mfu_dump.total_pages);
    view_info_add_field(app->info, "Pages", buf, COLOR_GREEN);

    snprintf(buf, sizeof(buf), "%02X %02X",
             app->mfu_dump.card.atqa[0], app->mfu_dump.card.atqa[1]);
    view_info_add_field(app->info, "ATQA", buf, COLOR_SECONDARY);
}

void nfc_scene_read_result_on_enter(void *ctx) {
    NfcApp *app = ctx;

    view_info_reset(app->info);
    view_info_set_header(app->info, "Card Found", COLOR_CYAN);

    if (app->card_type == NFC_CARD_MFU)
        show_mfu_result(app);
    else
        show_mfc_result(app);

    view_info_add_separator(app->info);

    view_info_add_button_row(app->info,
        LV_SYMBOL_REFRESH " Retry", lv_color_hex(0x2A2A2A), retry_cb, app,
        "More " LV_SYMBOL_RIGHT,    COLOR_CYAN,              more_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_read_result_on_event(void *ctx, SceneEvent event) {
    NfcApp *app = ctx;
    if(event.type == SceneEventTypeCustom) {
        if(event.event == NFC_EVT_RETRY) {
            scene_manager_search_and_switch_to_previous_scene(
                &app->scene_mgr, nfc_SCENE_Read);
            return true;
        }
        if(event.event == NFC_EVT_MORE) {
            scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_ReadMenu);
            return true;
        }
    }
    return false;
}

void nfc_scene_read_result_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_info_reset(app->info);
}
