/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui_data.h"
#include <string.h>

static struct notif_storage notifs[MAX_NOTIFICATIONS];
static int notif_count;

static struct forecast_storage forecast[MAX_FORECAST_DAYS];
static int forecast_count;

void ui_data_add_notification(uint8_t icon_id, const char *title,
			      const char *body)
{
	if (notif_count < MAX_NOTIFICATIONS) {
		notif_count++;
	}

	/* Shift existing entries down, newest at index 0 */
	for (int i = notif_count - 1; i > 0; i--) {
		notifs[i] = notifs[i - 1];
	}

	notifs[0].icon_id = icon_id;
	strncpy(notifs[0].title, title, sizeof(notifs[0].title) - 1);
	notifs[0].title[sizeof(notifs[0].title) - 1] = '\0';
	strncpy(notifs[0].body, body, sizeof(notifs[0].body) - 1);
	notifs[0].body[sizeof(notifs[0].body) - 1] = '\0';
}

void ui_data_clear_notifications(void)
{
	notif_count = 0;
	memset(notifs, 0, sizeof(notifs));
}

const struct notif_storage *ui_data_get_notification(int index)
{
	if (index < 0 || index >= notif_count) {
		return NULL;
	}
	return &notifs[index];
}

int ui_data_get_notification_count(void)
{
	return notif_count;
}

void ui_data_set_forecast(const struct forecast_storage *days, int count)
{
	if (count > MAX_FORECAST_DAYS) {
		count = MAX_FORECAST_DAYS;
	}
	memcpy(forecast, days, count * sizeof(struct forecast_storage));
	forecast_count = count;
}

const struct forecast_storage *ui_data_get_forecast(void)
{
	return forecast;
}

int ui_data_get_forecast_count(void)
{
	return forecast_count;
}
