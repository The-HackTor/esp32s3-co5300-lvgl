#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"
#include <stdio.h>

/*
 * Card Info -- pure info display, no action buttons.
 * Save/Emulate are in the contextual ReadMenu or SavedMenu.
 */
void nfc_scene_card_info_on_enter(void *ctx) {
    NfcApp *app = ctx;
    char buf[128];

    view_info_reset(app->info);
    view_info_set_header(app->info, "Card Info", COLOR_CYAN);

    if (app->card_type == NFC_CARD_MFU) {
        view_info_add_field(app->info, "Type",
                            mfu_type_str(app->mfu_dump.type), COLOR_CYAN);

        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X",
                 app->mfu_dump.card.uid[0], app->mfu_dump.card.uid[1],
                 app->mfu_dump.card.uid[2], app->mfu_dump.card.uid[3],
                 app->mfu_dump.card.uid[4], app->mfu_dump.card.uid[5],
                 app->mfu_dump.card.uid[6]);
        view_info_add_field(app->info, "UID", buf, COLOR_PRIMARY);

        snprintf(buf, sizeof(buf), "0x%02X", app->mfu_dump.card.sak);
        view_info_add_field(app->info, "SAK", buf, COLOR_SECONDARY);

        snprintf(buf, sizeof(buf), "%u", app->mfu_dump.total_pages);
        view_info_add_field(app->info, "Pages", buf, COLOR_SECONDARY);

        view_info_add_separator(app->info);

        char hex_buf[512];
        int off = 0;
        uint16_t show = app->mfu_dump.total_pages < 8 ? app->mfu_dump.total_pages : 8;
        for (uint16_t p = 0; p < show; p++) {
            off += snprintf(hex_buf + off, sizeof(hex_buf) - off, "%02u: ", p);
            for (int i = 0; i < MFU_PAGE_SIZE; i++) {
                off += snprintf(hex_buf + off, sizeof(hex_buf) - off, "%02X ",
                                app->mfu_dump.pages[p][i]);
            }
            if (p < show - 1)
                off += snprintf(hex_buf + off, sizeof(hex_buf) - off, "\n");
        }
        view_info_add_text_block(app->info, hex_buf, FONT_MONO, COLOR_GREEN);
    } else {
        view_info_add_field(app->info, "Type",
                            mfc_type_str(app->dump.type), COLOR_CYAN);

        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X",
                 app->dump.card.uid[0], app->dump.card.uid[1],
                 app->dump.card.uid[2], app->dump.card.uid[3]);
        view_info_add_field(app->info, "UID", buf, COLOR_PRIMARY);

        snprintf(buf, sizeof(buf), "0x%02X", app->dump.card.sak);
        view_info_add_field(app->info, "SAK", buf, COLOR_SECONDARY);

        snprintf(buf, sizeof(buf), "%02X %02X",
                 app->dump.card.atqa[0], app->dump.card.atqa[1]);
        view_info_add_field(app->info, "ATQA", buf, COLOR_SECONDARY);

        int total_sectors = mfc_total_sectors(app->dump.type);
        snprintf(buf, sizeof(buf), "%d/%d",
                 mfc_sectors_read(&app->dump), total_sectors);
        view_info_add_field(app->info, "Sectors Read", buf, COLOR_GREEN);

        snprintf(buf, sizeof(buf), "%d/%d",
                 mfc_keys_found(&app->dump), total_sectors * 2);
        view_info_add_field(app->info, "Keys Found", buf, COLOR_GREEN);

        view_info_add_separator(app->info);

        char hex_buf[512];
        int off = 0;
        for(int b = 0; b < 4 && b < MFC_MAX_BLOCKS; b++) {
            off += snprintf(hex_buf + off, sizeof(hex_buf) - off, "%02d: ", b);
            bool read = app->dump.block_read[b];
            for(int i = 0; i < MFC_BLOCK_SIZE; i++) {
                if (read) {
                    off += snprintf(hex_buf + off, sizeof(hex_buf) - off,
                                    "%02X ", app->dump.blocks[b][i]);
                } else {
                    off += snprintf(hex_buf + off, sizeof(hex_buf) - off,
                                    "?? ");
                }
            }
            if(b < 3)
                off += snprintf(hex_buf + off, sizeof(hex_buf) - off, "\n");
        }
        view_info_add_text_block(app->info, hex_buf, FONT_MONO, COLOR_GREEN);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_card_info_on_event(void *ctx, SceneEvent event) {
    (void)ctx;
    (void)event;
    return false;
}

void nfc_scene_card_info_on_exit(void *ctx) {
    NfcApp *app = ctx;
    view_info_reset(app->info);
}
