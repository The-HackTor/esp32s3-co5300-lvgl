#include "nfc_app.h"
#include "nfc_scenes.h"
#include "app/app_manager.h"
#include "ui/styles.h"

enum {
	PROTO_4A, PROTO_4B, PROTO_V, PROTO_F,
	PROTO_DESFIRE, PROTO_PLUS,
	PROTO_ST25TB, PROTO_PICOPASS, PROTO_EMV,
	PROTO_SCRIPT, PROTO_ANALYTICS,
};

static void submenu_cb(void *context, uint32_t index)
{
	NfcApp *app = context;
	scene_manager_set_scene_state(&app->scene_mgr, nfc_SCENE_Protocols, index);

	switch (index) {
	case PROTO_4A:
	case PROTO_4B:
	case PROTO_V:
	case PROTO_F:
	case PROTO_DESFIRE:
	case PROTO_PLUS:
	case PROTO_ST25TB:
	case PROTO_PICOPASS:
	case PROTO_EMV:
		scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_ProtoScan);
		break;
	case PROTO_SCRIPT:
		scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Script);
		break;
	case PROTO_ANALYTICS:
		scene_manager_next_scene(&app->scene_mgr, nfc_SCENE_Analytics);
		break;
	}
}

void nfc_scene_protocols_on_enter(void *ctx)
{
	NfcApp *app = ctx;

	view_submenu_reset(app->submenu);
	view_submenu_set_header(app->submenu, "Protocols", COLOR_CYAN);

	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan ISO-4A",
			      COLOR_CYAN, PROTO_4A, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan Type B",
			      COLOR_CYAN, PROTO_4B, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan NFC-V",
			      COLOR_CYAN, PROTO_V, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan FeliCa",
			      COLOR_CYAN, PROTO_F, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan DESFire",
			      COLOR_ORANGE, PROTO_DESFIRE, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan Plus",
			      COLOR_ORANGE, PROTO_PLUS, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan ST25TB",
			      COLOR_BLUE, PROTO_ST25TB, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan iCLASS",
			      COLOR_BLUE, PROTO_PICOPASS, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_WIFI, "Scan EMV",
			      COLOR_ORANGE, PROTO_EMV, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_FILE, "Run Script",
			      COLOR_GREEN, PROTO_SCRIPT, submenu_cb, app);
	view_submenu_add_item(app->submenu, LV_SYMBOL_LIST, "Analytics",
			      COLOR_SECONDARY, PROTO_ANALYTICS, submenu_cb, app);

	view_submenu_set_selected_item(app->submenu,
		scene_manager_get_scene_state(&app->scene_mgr, nfc_SCENE_Protocols));

	view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewSubmenu);
}

bool nfc_scene_protocols_on_event(void *ctx, SceneEvent event)
{
	NfcApp *app = ctx;
	if (event.type == SceneEventTypeBack) {
		scene_manager_previous_scene(&app->scene_mgr);
		return true;
	}
	(void)app;
	return false;
}

void nfc_scene_protocols_on_exit(void *ctx)
{
	NfcApp *app = ctx;
	view_submenu_reset(app->submenu);
}
