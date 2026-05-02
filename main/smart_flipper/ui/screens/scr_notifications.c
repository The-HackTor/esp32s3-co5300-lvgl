#include "scr_notifications.h"
#include "ui/styles.h"
#include "ui/ui_data.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/widgets/status_bar.h"

static lv_obj_t *screen;
static lv_obj_t *list;
static lv_obj_t *empty_lbl;

/*
 * Compute a circular-aware card width.
 * dy: vertical pixel offset from the display centre.
 */
static int circ_card_width(int dy)
{
    int r = DISP_RADIUS - 24;
    if(dy < 0) dy = -dy;
    if(dy >= r) return 220;
    int32_t w = 2 * lv_sqrt32(r * r - dy * dy);
    if(w < 220) w = 220;
    if(w > DISP_W - 40) w = DISP_W - 40;
    return (int)w;
}

static const char *icon_for_id(uint8_t icon_id)
{
    switch(icon_id) {
    case 0:  return LV_SYMBOL_ENVELOPE;
    case 1:  return LV_SYMBOL_BELL;
    case 2:  return LV_SYMBOL_CALL;
    case 3:  return LV_SYMBOL_WARNING;
    default: return LV_SYMBOL_BELL;
    }
}

static void check_empty(void)
{
    if(lv_obj_get_child_count(list) == 0) {
        if(!empty_lbl) {
            empty_lbl = lv_label_create(screen);
            lv_label_set_text(empty_lbl, "No notifications");
            lv_obj_set_style_text_font(empty_lbl, FONT_MENU, 0);
            lv_obj_set_style_text_color(empty_lbl, COLOR_DIM, 0);
            lv_obj_center(empty_lbl);
        }
    }
}

static void dismiss_x_cb(void *var, int32_t val)
{
    lv_obj_set_style_translate_x((lv_obj_t *)var, val, 0);
}

static void dismiss_opa_cb(void *var, int32_t val)
{
    lv_obj_set_style_opa((lv_obj_t *)var, val, 0);
}

static void dismiss_anim_done(lv_anim_t *a)
{
    lv_obj_t *card = a->var;
    lv_anim_delete(card, dismiss_opa_cb);
    lv_obj_delete(card);
    check_empty();
}

static void card_gesture(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if(dir == LV_DIR_LEFT) {
        lv_obj_t *card = lv_event_get_current_target(e);
        lv_event_stop_bubbling(e);
        lv_indev_wait_release(lv_indev_active());

        lv_anim_t a_x;
        lv_anim_init(&a_x);
        lv_anim_set_var(&a_x, card);
        lv_anim_set_values(&a_x, 0, -DISP_W);
        lv_anim_set_duration(&a_x, 250);
        lv_anim_set_exec_cb(&a_x, dismiss_x_cb);
        lv_anim_set_completed_cb(&a_x, dismiss_anim_done);
        lv_anim_start(&a_x);

        lv_anim_t a_o;
        lv_anim_init(&a_o);
        lv_anim_set_var(&a_o, card);
        lv_anim_set_values(&a_o, 255, 0);
        lv_anim_set_duration(&a_o, 250);
        lv_anim_set_exec_cb(&a_o, dismiss_opa_cb);
        lv_anim_start(&a_o);
    }
}

static void on_init(void)
{
    screen = lv_obj_create(NULL);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, DISP_W, DISP_H);
    nav_install_gesture(screen);
    empty_lbl = NULL;

    status_bar_create(screen, "Notifications", COLOR_YELLOW);

    list = lv_obj_create(screen);
    lv_obj_set_size(list, DISP_W - 32, DISP_H - 100);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 76);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_style_pad_top(list, 10, 0);
    lv_obj_set_style_pad_bottom(list, 50, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    int count = ui_data_get_notification_count();
    if(count == 0) {
        check_empty();
    } else {
        /*
         * Approximate each card's vertical offset from centre for
         * circular width calculation.
         * List starts about -110 px from centre; each card ~100 px apart.
         */
        for(int i = 0; i < count; i++) {
            const struct notif_storage *n = ui_data_get_notification(i);
            if(!n) continue;

            int dy = -110 + i * 100;
            int card_w = circ_card_width(dy);

            lv_obj_t *card = lv_obj_create(list);
            lv_obj_add_style(card, &style_card, 0);
            lv_obj_set_size(card, card_w, LV_SIZE_CONTENT);
            lv_obj_set_style_min_height(card, 84, 0);
            lv_obj_set_style_pad_all(card, 14, 0);
            lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
            lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(card, card_gesture, LV_EVENT_GESTURE, NULL);

            lv_obj_t *hdr = lv_label_create(card);
            lv_label_set_text_fmt(hdr, "%s  %s", icon_for_id(n->icon_id), n->title);
            lv_obj_set_style_text_color(hdr, COLOR_BLUE, 0);
            lv_obj_set_style_text_font(hdr, FONT_MENU, 0);
            lv_obj_align(hdr, LV_ALIGN_TOP_LEFT, 0, 0);

            lv_obj_t *body = lv_label_create(card);
            lv_label_set_text(body, n->body);
            lv_obj_set_style_text_color(body, COLOR_SECONDARY, 0);
            lv_obj_set_style_text_font(body, FONT_BODY, 0);
            lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(body, lv_pct(100));
            lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 32);
        }
    }
}

static void on_enter(void) { lv_screen_load(screen); }
static void on_leave(void) {}
static lv_obj_t *get_screen(void) { return screen; }

void scr_notifications_register(void)
{
    static const AppDescriptor desc = {
        .id = APP_ID_NOTIFICATIONS,
        .name = "Notifs",
        .icon = LV_SYMBOL_BELL,
        .color = {.red = 0xFF, .green = 0xD7, .blue = 0x40},
        .on_init = on_init,
        .on_enter = on_enter,
        .on_leave = on_leave,
        .get_screen = get_screen,
        .get_scene_manager = NULL,
    };
    app_manager_register(&desc);
}
