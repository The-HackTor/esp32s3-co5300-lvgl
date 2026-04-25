/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reactive data subjects for the smartwatch UI.
 * Follows the same pattern as chronos-osw: LVGL lv_subject_t observers
 * drive automatic UI updates when values change.
 */

#ifndef UI_SUBJECTS_H
#define UI_SUBJECTS_H

#include <lvgl.h>

/* Time / Date */
extern lv_subject_t subject_hour;          /* int: 0-23              */
extern lv_subject_t subject_minute;        /* int: 0-59              */
extern lv_subject_t subject_second;        /* int: 0-59              */
extern lv_subject_t subject_time;          /* string: "12:45"        */
extern lv_subject_t subject_date;          /* string: "Mon, Mar 17"  */
extern lv_subject_t subject_am_pm;         /* string: "AM" / "PM"    */

/* Health / Activity */
extern lv_subject_t subject_steps;         /* int: step count        */
extern lv_subject_t subject_calories;      /* int: kcal              */
extern lv_subject_t subject_distance;      /* string: "1.3"          */
extern lv_subject_t subject_distance_unit; /* string: "km"           */
extern lv_subject_t subject_heart_rate;    /* int: BPM               */
extern lv_subject_t subject_active_min;    /* int: minutes           */

/* System */
extern lv_subject_t subject_battery;       /* int: 0-100 %           */
extern lv_subject_t subject_bluetooth;     /* int: 0=off 1=on 2=conn */

/* Weather */
extern lv_subject_t subject_temperature;   /* int: degrees           */
extern lv_subject_t subject_weather_icon;  /* int: -1..7             */
extern lv_subject_t subject_temp_unit;     /* string: "C" / "F"    */
extern lv_subject_t subject_weather_city;      /* string: city name  */
extern lv_subject_t subject_weather_condition; /* string: condition  */
extern lv_subject_t subject_temp_high;     /* int: high temp         */
extern lv_subject_t subject_temp_low;      /* int: low temp          */
extern lv_subject_t subject_humidity;      /* int: 0-100 %           */
extern lv_subject_t subject_forecast_update;   /* int: bump to redraw */

/* Music */
extern lv_subject_t subject_music_title;   /* string: track title    */
extern lv_subject_t subject_music_artist;  /* string: artist name    */
extern lv_subject_t subject_music_playing; /* int: 0=paused 1=playing */

/* Notifications */
extern lv_subject_t subject_notif_update;  /* int: bump to redraw    */

/* Brightness */
extern lv_subject_t subject_brightness;    /* int: 10-255            */

/* OTA */
extern lv_subject_t subject_ota_progress;  /* int: 0-100, -1=inactive */

void ui_subjects_init(void);

#endif /* UI_SUBJECTS_H */
