/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui_subjects.h"

/* Time / Date */
lv_subject_t subject_hour;
lv_subject_t subject_minute;
lv_subject_t subject_second;
lv_subject_t subject_time;
lv_subject_t subject_date;
lv_subject_t subject_am_pm;

/* Health / Activity */
lv_subject_t subject_steps;
lv_subject_t subject_calories;
lv_subject_t subject_distance;
lv_subject_t subject_distance_unit;
lv_subject_t subject_heart_rate;
lv_subject_t subject_active_min;

/* System */
lv_subject_t subject_battery;
lv_subject_t subject_bluetooth;

/* Weather */
lv_subject_t subject_temperature;
lv_subject_t subject_weather_icon;
lv_subject_t subject_temp_unit;
lv_subject_t subject_weather_city;
lv_subject_t subject_weather_condition;
lv_subject_t subject_temp_high;
lv_subject_t subject_temp_low;
lv_subject_t subject_humidity;
lv_subject_t subject_forecast_update;

/* Music */
lv_subject_t subject_music_title;
lv_subject_t subject_music_artist;
lv_subject_t subject_music_playing;

/* Notifications */
lv_subject_t subject_notif_update;

/* Brightness */
lv_subject_t subject_brightness;

/* OTA */
lv_subject_t subject_ota_progress;

/*
 * String subjects need: buf, prev_buf, size, initial_value.
 * buf and prev_buf must remain valid for the lifetime of the subject.
 */
static char buf_time[8]               = "12:00";
static char prev_time[8];
static char buf_date[20]              = "Mon, Mar 17";
static char prev_date[20];
static char buf_am_pm[4]              = "PM";
static char prev_am_pm[4];
static char buf_distance[8]           = "0.0";
static char prev_distance[8];
static char buf_distance_unit[4]      = "km";
static char prev_distance_unit[4];
static char buf_temp_unit[4]          = "\xC2\xB0""C";  /* °C */
static char prev_temp_unit[4];
static char buf_weather_city[32]      = "";
static char prev_weather_city[32];
static char buf_weather_cond[32]      = "";
static char prev_weather_cond[32];
static char buf_music_title[64]       = "";
static char prev_music_title[64];
static char buf_music_artist[64]      = "";
static char prev_music_artist[64];

void ui_subjects_init(void)
{
	/* Time */
	lv_subject_init_int(&subject_hour, 12);
	lv_subject_init_int(&subject_minute, 0);
	lv_subject_init_int(&subject_second, 0);
	lv_subject_init_string(&subject_time, buf_time, prev_time,
			       sizeof(buf_time), buf_time);
	lv_subject_init_string(&subject_date, buf_date, prev_date,
			       sizeof(buf_date), buf_date);
	lv_subject_init_string(&subject_am_pm, buf_am_pm, prev_am_pm,
			       sizeof(buf_am_pm), buf_am_pm);

	/* Health */
	lv_subject_init_int(&subject_steps, 0);
	lv_subject_init_int(&subject_calories, 0);
	lv_subject_init_string(&subject_distance, buf_distance, prev_distance,
			       sizeof(buf_distance), buf_distance);
	lv_subject_init_string(&subject_distance_unit, buf_distance_unit,
			       prev_distance_unit,
			       sizeof(buf_distance_unit), buf_distance_unit);
	lv_subject_init_int(&subject_heart_rate, 0);
	lv_subject_init_int(&subject_active_min, 0);

	/* System */
	lv_subject_init_int(&subject_battery, 0);
	lv_subject_init_int(&subject_bluetooth, 0);
	lv_subject_init_int(&subject_brightness, 128);

	/* Weather */
	lv_subject_init_int(&subject_temperature, -128);
	lv_subject_init_int(&subject_weather_icon, -1);
	lv_subject_init_string(&subject_temp_unit, buf_temp_unit,
			       prev_temp_unit,
			       sizeof(buf_temp_unit), buf_temp_unit);
	lv_subject_init_string(&subject_weather_city, buf_weather_city,
			       prev_weather_city,
			       sizeof(buf_weather_city), buf_weather_city);
	lv_subject_init_string(&subject_weather_condition, buf_weather_cond,
			       prev_weather_cond,
			       sizeof(buf_weather_cond), buf_weather_cond);
	lv_subject_init_int(&subject_temp_high, -128);
	lv_subject_init_int(&subject_temp_low, -128);
	lv_subject_init_int(&subject_humidity, 0);
	lv_subject_init_int(&subject_forecast_update, 0);

	/* Music */
	lv_subject_init_string(&subject_music_title, buf_music_title,
			       prev_music_title,
			       sizeof(buf_music_title), buf_music_title);
	lv_subject_init_string(&subject_music_artist, buf_music_artist,
			       prev_music_artist,
			       sizeof(buf_music_artist), buf_music_artist);
	lv_subject_init_int(&subject_music_playing, 0);

	/* Notifications */
	lv_subject_init_int(&subject_notif_update, 0);

	/* OTA */
	lv_subject_init_int(&subject_ota_progress, -1);
}
