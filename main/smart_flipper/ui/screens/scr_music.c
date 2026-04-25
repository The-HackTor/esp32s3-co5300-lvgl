#include "scr_music.h"
#include "ui/styles.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/widgets/status_bar.h"
#include "bridge/bridge.h"
#include <app/bridge_protocol.h>

static lv_obj_t *screen;
static bool playing;
static lv_obj_t *btn_play_lbl;
static lv_obj_t *arc_progress;
static lv_obj_t *lbl_elapsed;
static lv_obj_t *lbl_total;
static lv_timer_t *progress_timer;
static int elapsed_sec;

static void play_cb(lv_event_t *e)
{
    (void)e;
    playing = !playing;
    lv_label_set_text(btn_play_lbl, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    uint8_t cmd = MUSIC_PLAY_PAUSE;
    bridge_send(BRIDGE_MSG_MUSIC_CTRL, &cmd, 1);
}

static void prev_cb(lv_event_t *e)
{
    (void)e;
    uint8_t cmd = MUSIC_PREV;
    bridge_send(BRIDGE_MSG_MUSIC_CTRL, &cmd, 1);
}

static void next_cb(lv_event_t *e)
{
    (void)e;
    uint8_t cmd = MUSIC_NEXT;
    bridge_send(BRIDGE_MSG_MUSIC_CTRL, &cmd, 1);
}

static void vol_up_cb(lv_event_t *e)
{
    (void)e;
    uint8_t cmd = MUSIC_VOL_UP;
    bridge_send(BRIDGE_MSG_MUSIC_CTRL, &cmd, 1);
}

static void vol_dn_cb(lv_event_t *e)
{
    (void)e;
    uint8_t cmd = MUSIC_VOL_DOWN;
    bridge_send(BRIDGE_MSG_MUSIC_CTRL, &cmd, 1);
}

static void progress_tick(lv_timer_t *t)
{
    (void)t;
    if(!playing) return;
    elapsed_sec++;
    if(elapsed_sec > 217) elapsed_sec = 0;
    lv_arc_set_value(arc_progress, elapsed_sec);
    lv_label_set_text_fmt(lbl_elapsed, "%d:%02d", elapsed_sec / 60, elapsed_sec % 60);
}

static lv_obj_t *create_ctrl(lv_obj_t *parent, const char *sym, int size,
                              lv_color_t color, lv_event_cb_t cb,
                              const lv_font_t *font, int x, int y)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, COLOR_RING_BG, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_align(btn, LV_ALIGN_CENTER, x, y);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, sym);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_center(lbl);

    return btn;
}

static void on_init(void)
{
    screen = lv_obj_create(NULL);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, DISP_W, DISP_H);
    nav_install_gesture(screen);
    playing = false;
    elapsed_sec = 47;

    status_bar_create(screen, "Music", COLOR_MAGENTA);

    /* ---- progress arc around the full display ---- */
    arc_progress = lv_arc_create(screen);
    lv_obj_set_size(arc_progress, DISP_W - 36, DISP_H - 36);
    lv_obj_center(arc_progress);
    lv_arc_set_range(arc_progress, 0, 217);
    lv_arc_set_value(arc_progress, elapsed_sec);
    lv_arc_set_bg_angles(arc_progress, 0, 360);
    lv_arc_set_rotation(arc_progress, 270);
    lv_obj_remove_style(arc_progress, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc_progress, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_progress, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_progress, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_progress, COLOR_MAGENTA, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_progress, true, LV_PART_INDICATOR);

    /* ---- track info ---- */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Midnight City");
    lv_obj_set_style_text_font(title, FONT_MENU, 0);      /* 24 px -- was FONT_TITLE */
    lv_obj_set_style_text_color(title, COLOR_PRIMARY, 0);
    lv_obj_set_style_max_width(title, 340, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -78);

    lv_obj_t *artist = lv_label_create(screen);
    lv_label_set_text(artist, "M83");
    lv_obj_set_style_text_font(artist, FONT_BODY, 0);     /* 18 px */
    lv_obj_set_style_text_color(artist, COLOR_SECONDARY, 0);
    lv_obj_align(artist, LV_ALIGN_CENTER, 0, -48);

    lv_obj_t *album = lv_label_create(screen);
    lv_label_set_text(album, "Hurry Up, We're Dreaming");
    lv_obj_set_style_text_font(album, FONT_BODY, 0);      /* 18 px -- was FONT_SMALL */
    lv_obj_set_style_text_color(album, COLOR_DIM, 0);
    lv_obj_set_style_max_width(album, 320, 0);
    lv_label_set_long_mode(album, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(album, LV_ALIGN_CENTER, 0, -22);

    /* ---- playback controls (oversized touch targets) ---- */
    create_ctrl(screen, LV_SYMBOL_PREV, 64, COLOR_PRIMARY, prev_cb,
                FONT_MENU, -100, 30);

    lv_obj_t *play = create_ctrl(screen, LV_SYMBOL_PLAY, 80, COLOR_MAGENTA,
                                  play_cb, FONT_MENU, 0, 30);
    btn_play_lbl = lv_obj_get_child(play, 0);
    lv_obj_set_style_text_font(btn_play_lbl, FONT_TITLE, 0);

    create_ctrl(screen, LV_SYMBOL_NEXT, 64, COLOR_PRIMARY, next_cb,
                FONT_MENU, 100, 30);

    /* ---- volume controls ---- */
    create_ctrl(screen, LV_SYMBOL_VOLUME_MID, 56, COLOR_GREEN, vol_dn_cb,
                FONT_BODY, -60, 108);
    create_ctrl(screen, LV_SYMBOL_VOLUME_MAX, 56, COLOR_GREEN, vol_up_cb,
                FONT_BODY, 60, 108);

    /* ---- elapsed / total time ---- */
    lbl_elapsed = lv_label_create(screen);
    lv_label_set_text_fmt(lbl_elapsed, "%d:%02d", elapsed_sec / 60, elapsed_sec % 60);
    lv_obj_set_style_text_font(lbl_elapsed, FONT_BODY, 0);   /* 18 px -- was FONT_MONO */
    lv_obj_set_style_text_color(lbl_elapsed, COLOR_SECONDARY, 0);
    lv_obj_align(lbl_elapsed, LV_ALIGN_BOTTOM_MID, -44, -52);

    lbl_total = lv_label_create(screen);
    lv_label_set_text(lbl_total, "3:37");
    lv_obj_set_style_text_font(lbl_total, FONT_BODY, 0);     /* 18 px -- was FONT_MONO */
    lv_obj_set_style_text_color(lbl_total, COLOR_DIM, 0);
    lv_obj_align(lbl_total, LV_ALIGN_BOTTOM_MID, 44, -52);
}

static void on_enter(void)
{
    lv_screen_load(screen);
    progress_timer = lv_timer_create(progress_tick, 1000, NULL);
}

static void on_leave(void)
{
    if(progress_timer) {
        lv_timer_delete(progress_timer);
        progress_timer = NULL;
    }
}

static lv_obj_t *get_screen(void) { return screen; }

void scr_music_register(void)
{
    static const AppDescriptor desc = {
        .id = APP_ID_MUSIC,
        .name = "Music",
        .icon = LV_SYMBOL_AUDIO,
        .color = {.red = 0xE0, .green = 0x40, .blue = 0xFB},
        .on_init = on_init,
        .on_enter = on_enter,
        .on_leave = on_leave,
        .get_screen = get_screen,
        .get_scene_manager = NULL,
    };
    app_manager_register(&desc);
}
