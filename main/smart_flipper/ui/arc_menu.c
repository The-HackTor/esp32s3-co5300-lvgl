#include "arc_menu.h"
#include "styles.h"
#include "widgets/barrel_list.h"
#include "app/app_manager.h"

static lv_obj_t *overlay;
static lv_obj_t *list;
static bool visible;

static void bg_clicked(lv_event_t *e)
{
    (void)e;
    arc_menu_hide();
}

static void menu_gesture(lv_event_t *e)
{
    (void)e;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    if(dir == LV_DIR_RIGHT || dir == LV_DIR_BOTTOM) {
        arc_menu_hide();
        lv_indev_wait_release(lv_indev_active());
    }
}

static void list_scroll_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    int scroll_y = lv_obj_get_scroll_y(obj);
    if(scroll_y < -50) {
        arc_menu_hide();
    }
}

static void item_clicked(lv_event_t *e)
{
    AppId id = (AppId)(uintptr_t)lv_event_get_user_data(e);
    arc_menu_hide();
    app_manager_launch(id);
}

void arc_menu_init(void)
{
    overlay = NULL;
    list = NULL;
    visible = false;
}

static void populate_list(void)
{
    uint32_t count = app_manager_get_app_count();
    for(uint32_t i = 0; i < count; i++) {
        const AppDescriptor *desc = app_manager_get_app_by_index(i);
        if(!desc) continue;

        /* Separator line before each item (except the first) */
        if(i > 0) {
            lv_obj_t *line = lv_obj_create(list);
            lv_obj_set_size(line, lv_pct(70), 1);
            lv_obj_set_style_bg_color(line, lv_color_hex(0x444444), 0);
            lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(line, 0, 0);
            lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        }

        /* Transparent row -- no card background */
        lv_obj_t *btn = lv_obj_create(list);
        lv_obj_set_size(btn, lv_pct(100), 72);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, COLOR_PRIMARY, LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_add_event_cb(btn, item_clicked, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)desc->id);

        lv_obj_t *icn = lv_label_create(btn);
        lv_label_set_text(icn, desc->icon);
        lv_obj_set_style_text_color(icn, desc->color, 0);
        lv_obj_set_style_text_font(icn, FONT_TIME, 0);
        lv_obj_align(icn, LV_ALIGN_LEFT_MID, 20, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, desc->name);
        lv_obj_set_style_text_color(lbl, COLOR_PRIMARY, 0);
        lv_obj_set_style_text_font(lbl, FONT_TIME, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 100, 0);
    }
}

void arc_menu_show(void)
{
    if(visible) return;
    visible = true;

    if(!overlay) {
        /* First time: create persistent overlay */
        overlay = lv_obj_create(lv_layer_top());
        lv_obj_set_size(overlay, DISP_W, DISP_H);
        lv_obj_set_style_bg_color(overlay, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(overlay, 0, 0);
        lv_obj_set_style_pad_all(overlay, 0, 0);
        lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(overlay, bg_clicked, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(overlay, menu_gesture, LV_EVENT_GESTURE, NULL);

        list = barrel_list_create(overlay);
        lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLL_ELASTIC);
        lv_obj_add_event_cb(list, list_scroll_cb, LV_EVENT_SCROLL, NULL);
    }

    /* Rebuild item list (apps may have changed) */
    lv_obj_clean(list);
    populate_list();
    barrel_list_refresh(list);

    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);

    /* Slide-up entrance. Deliberately not an opacity fade: animating
     * style_opa on a parent with children forces LVGL to render the whole
     * subtree to an ARGB8888 compositor layer (~870 KB for 466x466), which
     * under post-boot TLSF fragmentation trips LV_ASSERT_MALLOC. A position
     * animation just translates the overlay each frame; every child stays
     * fully opaque, the dirty region is the slide strip, and no layer
     * buffer is needed. The old comment about clip_corner/radius causing
     * draw-buffer underflow at non-zero Y applied to the 21 KB partial
     * buffer era — with the current full-frame PSRAM buffers there are no
     * chunk boundaries for a rounded corner mask to straddle. */
    lv_obj_set_y(overlay, DISP_H);
    lv_obj_set_style_opa(overlay, LV_OPA_COVER, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_values(&a, DISP_H, 0);
    lv_anim_set_duration(&a, 150);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void arc_menu_hide(void)
{
    if(!visible) return;
    visible = false;
    if(overlay) {
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

bool arc_menu_is_visible(void)
{
    return visible;
}
