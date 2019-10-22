#include <batterymonitoringservice.h>
#include <database.h>
#include <Elementary.h>
#include <service_app.h>
#include <dlog.h>
#include <device/battery.h>
#include <time.h>
#include <app_alarm.h>

struct appdata {
	int alarm_id;
};
typedef struct appdata appdata_s;

void store_battery_info() {
	time_t raw_time;
	struct tm* time_info;
	int error_code;
	int battery_percentage;
	bool battery_is_charging;
	int battery_is_charging_int;
	char time_text[20] = { "\0" };

	// get date and time
	time(&raw_time);
	time_info = localtime(&raw_time);

	// battery percentage
	error_code = device_battery_get_percent(&battery_percentage);
	if (error_code != DEVICE_ERROR_NONE) {
		battery_percentage = -1;
	}

	// battery charging
	error_code = device_battery_is_charging(&battery_is_charging);
	if (error_code != DEVICE_ERROR_NONE) {
		battery_is_charging_int = -1;
	} else {
		battery_is_charging_int = battery_is_charging ? 1 : 0;
	}

	// log metrics
	strftime(time_text, sizeof(time_text), "%Y-%m-%dT%H:%M:%S", time_info);
	insert_sample(time_text, battery_percentage, battery_is_charging_int);
}

bool init_alarm(appdata_s *ad) {
	// check if the alarm already exist
	if (ad->alarm_id == 0) {
		// Although this is inexact, the alarm will not fire before this time
		int DELAY = 1;
		// This value does not guarantee the accuracy. The actual interval is calculated by the OS. The minimum value is 600sec
		int REMIND = 600;
		int alarm_id;
		app_control_h app_control = NULL;
		app_control_create(&app_control);
		app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
		app_control_set_app_id(app_control,
				"it.silviomessi.batterymonitoringservice");
		alarm_schedule_after_delay(app_control, DELAY, REMIND, &alarm_id);
		ad->alarm_id = alarm_id;
	}
	return true;
}

bool app_create(void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_create");
	// data control setup
	initialize_datacontrol_provider();
	return true;
}

void app_control(app_control_h app_control, void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_control");
	appdata_s *ad = (appdata_s *) data;

	/* the info about the battery are stored:
	 * - first time the service is started
	 * - when the alert open the service
	 * - when the consumer application open the service
	 */
	store_battery_info();

	/* setup the alarm here
	 * if, for any reason, the alert has been removed let's create it again
	 * TODO: what happen when the service is closed or the device is rebooted?
	 */
	init_alarm(ad);

	// print alarm status
	int error_code;
	struct tm date;
	time_t time_current;
	error_code = alarm_get_scheduled_date(ad->alarm_id, &date);
	if (error_code != ALARM_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Get time Error: %d ", error_code);
	} else {
		time_current = mktime(&date);
		dlog_print(DLOG_INFO, LOG_TAG, "Registered alarm: %d on date: %s",
				ad->alarm_id, ctime(&time_current));
	}
}

void app_terminate(void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_terminate");
	return;
}

int main(int argc, char *argv[]) {
	int ret;
	appdata_s ad = { 0, };

	service_app_lifecycle_callback_s event_callback = { 0, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.app_control = app_control;

	ret = service_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "main() failed. err = %d", ret);

	return ret;
}
