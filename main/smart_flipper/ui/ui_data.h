/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Storage for notification and forecast data received from phone.
 * Ring buffer for notifications, flat array for 5-day forecast.
 */

#ifndef UI_DATA_H
#define UI_DATA_H

#include <stdint.h>

#define MAX_NOTIFICATIONS 10
#define MAX_FORECAST_DAYS 5

struct notif_storage {
	uint8_t icon_id;
	char title[64];
	char body[64];
};

struct forecast_storage {
	uint8_t weekday;
	uint8_t icon;
	int8_t temp_high;
	int8_t temp_low;
};

void ui_data_add_notification(uint8_t icon_id, const char *title,
			      const char *body);
void ui_data_clear_notifications(void);
const struct notif_storage *ui_data_get_notification(int index);
int ui_data_get_notification_count(void);

void ui_data_set_forecast(const struct forecast_storage *days, int count);
const struct forecast_storage *ui_data_get_forecast(void);
int ui_data_get_forecast_count(void);

#endif /* UI_DATA_H */
