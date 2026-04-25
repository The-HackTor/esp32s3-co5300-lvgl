#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"

void nfc_scene_proto_scan_on_enter(void *ctx)
{
	NfcApp *app = ctx;

	view_info_reset(app->info);
	view_info_set_header(app->info, "Protocol Scan", COLOR_CYAN);
	view_info_add_text_block(app->info,
			   "Place card near device.\n\n"
			   "Protocol-specific scan not wired to UI yet.\n"
			   "Use nfc_relay console (menu options 8, 9, 0, v,\n"
			   "f, i, e) for B/V/F/DESFire/EMV/iCLASS/ST25TB.",
			   FONT_BODY, COLOR_SECONDARY);

	view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_proto_scan_on_event(void *ctx, SceneEvent event)
{
	NfcApp *app = ctx;
	if (event.type == SceneEventTypeBack) {
		scene_manager_previous_scene(&app->scene_mgr);
		return true;
	}
	return false;
}

void nfc_scene_proto_scan_on_exit(void *ctx)
{
	NfcApp *app = ctx;
	view_info_reset(app->info);
}
