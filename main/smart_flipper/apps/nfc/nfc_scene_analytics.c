#include "nfc_app.h"
#include "nfc_scenes.h"
#include "ui/styles.h"

void nfc_scene_analytics_on_enter(void *ctx)
{
	NfcApp *app = ctx;

	view_info_reset(app->info);
	view_info_set_header(app->info, "Analytics", COLOR_SECONDARY);
	view_info_add_text_block(app->info,
			   "Per-event log at /lfs/nfc/events.log.\n\n"
			   "Use nfc_relay console 'analytics' for counts\n"
			   "per kind / tech. UI aggregation TBD.",
			   FONT_BODY, COLOR_SECONDARY);

	view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewInfo);
}

bool nfc_scene_analytics_on_event(void *ctx, SceneEvent event)
{
	NfcApp *app = ctx;
	if (event.type == SceneEventTypeBack) {
		scene_manager_previous_scene(&app->scene_mgr);
		return true;
	}
	return false;
}

void nfc_scene_analytics_on_exit(void *ctx)
{
	NfcApp *app = ctx;
	view_info_reset(app->info);
}
