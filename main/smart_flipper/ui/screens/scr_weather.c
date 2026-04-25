#include "scr_weather.h"
#include "ui/styles.h"
#include "ui/ui_subjects.h"
#include "ui/ui_data.h"
#include "app/app_manager.h"
#include "app/nav.h"
#include "ui/widgets/status_bar.h"

static lv_obj_t *screen;

static const char *weekday_short[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static lv_obj_t *create_detail(lv_obj_t *parent, const char *label,
                                const char *value, lv_color_t color)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 120, 76);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, FONT_TINY, 0);
    lv_obj_set_style_text_color(lbl, COLOR_DIM, 0);
    lv_obj_set_style_text_letter_space(lbl, 2, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *val = lv_label_create(cont);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_font(val, FONT_TITLE, 0);
    lv_obj_set_style_text_color(val, color, 0);
    lv_obj_align(val, LV_ALIGN_BOTTOM_MID, 0, -4);

    return cont;
}

static lv_obj_t *create_forecast_day(lv_obj_t *parent, const char *day,
                                      const char *hi, const char *lo,
                                      const char *icon)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_flex_grow(cont, 1);
    lv_obj_set_height(cont, 92);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *d = lv_label_create(cont);
    lv_label_set_text(d, day);
    lv_obj_set_style_text_font(d, FONT_SMALL, 0);
    lv_obj_set_style_text_color(d, COLOR_SECONDARY, 0);
    lv_obj_align(d, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *ic = lv_label_create(cont);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_font(ic, FONT_BODY, 0);
    lv_obj_set_style_text_color(ic, COLOR_YELLOW, 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, -2);

    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%s/%s", hi, lo);
    lv_obj_t *t = lv_label_create(cont);
    lv_label_set_text(t, buf);
    lv_obj_set_style_text_font(t, FONT_BODY, 0);      /* 18 px -- was FONT_MONO */
    lv_obj_set_style_text_color(t, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, 0);

    return cont;
}

static void on_init(void)
{
    screen = lv_obj_create(NULL);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, DISP_W, DISP_H);
    nav_install_gesture(screen);

    int temp = lv_subject_get_int(&subject_temperature);
    int hi   = lv_subject_get_int(&subject_temp_high);
    int lo   = lv_subject_get_int(&subject_temp_low);
    int hum  = lv_subject_get_int(&subject_humidity);

    status_bar_create(screen, "Weather", COLOR_BLUE);

    /* ---- prominent centred temperature ---- */
    lv_obj_t *t = lv_label_create(screen);
    lv_label_set_text_fmt(t, "%d\xc2\xb0" "C", temp);
    lv_obj_set_style_text_font(t, FONT_LARGE, 0);
    lv_obj_set_style_text_color(t, COLOR_PRIMARY, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, -72);

    lv_obj_t *cond = lv_label_create(screen);
    lv_label_set_text(cond, "Partly Cloudy");
    lv_obj_set_style_text_font(cond, FONT_BODY, 0);
    lv_obj_set_style_text_color(cond, COLOR_SECONDARY, 0);
    lv_obj_align(cond, LV_ALIGN_CENTER, 0, -32);

    /* ---- detail row: high / low / humidity ---- */
    lv_obj_t *details = lv_obj_create(screen);
    lv_obj_set_size(details, 390, 80);
    lv_obj_align(details, LV_ALIGN_CENTER, 0, 24);
    lv_obj_set_layout(details, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(details, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(details, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(details, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(details, 0, 0);
    lv_obj_remove_flag(details, LV_OBJ_FLAG_SCROLLABLE);

    char hi_buf[8], lo_buf[8], hum_buf[8];
    lv_snprintf(hi_buf,  sizeof(hi_buf),  "%d\xc2\xb0", hi);
    lv_snprintf(lo_buf,  sizeof(lo_buf),  "%d\xc2\xb0", lo);
    lv_snprintf(hum_buf, sizeof(hum_buf), "%d%%",        hum);

    create_detail(details, "HIGH",  hi_buf,  COLOR_ORANGE);
    create_detail(details, "LOW",   lo_buf,  COLOR_CYAN);
    create_detail(details, "HUMID", hum_buf, COLOR_BLUE);

    /* ---- divider ---- */
    lv_obj_t *divider = lv_obj_create(screen);
    lv_obj_set_size(divider, 340, 1);
    lv_obj_align(divider, LV_ALIGN_CENTER, 0, 72);
    lv_obj_set_style_bg_color(divider, COLOR_DIM, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_remove_flag(divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* ---- forecast row ---- */
    lv_obj_t *forecast = lv_obj_create(screen);
    lv_obj_set_size(forecast, 400, 100);
    lv_obj_align(forecast, LV_ALIGN_CENTER, 0, 130);
    lv_obj_set_layout(forecast, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(forecast, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(forecast, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(forecast, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(forecast, 0, 0);
    lv_obj_remove_flag(forecast, LV_OBJ_FLAG_SCROLLABLE);

    int fc_count = ui_data_get_forecast_count();
    if(fc_count > 0) {
        const struct forecast_storage *fc = ui_data_get_forecast();
        for(int i = 0; i < fc_count && i < 4; i++) {
            const char *day = (fc[i].weekday < 7) ? weekday_short[fc[i].weekday] : "???";
            char h[8], l[8];
            lv_snprintf(h, sizeof(h), "%d", fc[i].temp_high);
            lv_snprintf(l, sizeof(l), "%d", fc[i].temp_low);
            create_forecast_day(forecast, day, h, l, LV_SYMBOL_EYE_OPEN);
        }
    } else {
        create_forecast_day(forecast, "THU", "25", "18", LV_SYMBOL_EYE_OPEN);
        create_forecast_day(forecast, "FRI", "23", "16", LV_SYMBOL_CHARGE);
        create_forecast_day(forecast, "SAT", "27", "19", LV_SYMBOL_EYE_OPEN);
        create_forecast_day(forecast, "SUN", "24", "17", LV_SYMBOL_CHARGE);
    }
}

static void on_enter(void) { lv_screen_load(screen); }
static void on_leave(void) { /* nothing to stop */ }
static lv_obj_t *get_screen(void) { return screen; }

void scr_weather_register(void)
{
    static const AppDescriptor desc = {
        .id = APP_ID_WEATHER,
        .name = "Weather",
        .icon = LV_SYMBOL_EYE_OPEN,
        .color = {.red = 0x44, .green = 0x8A, .blue = 0xFF},
        .on_init = on_init,
        .on_enter = on_enter,
        .on_leave = on_leave,
        .get_screen = get_screen,
        .get_scene_manager = NULL,
    };
    app_manager_register(&desc);
}
