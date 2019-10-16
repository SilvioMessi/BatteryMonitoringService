#include <batterymonitoringservice.h>
#include <database.h>
#include <Elementary.h>
#include <service_app.h>
#include <dlog.h>
#include <device/battery.h>
#include <time.h>

static Eina_Bool timeout_func(void *data) {
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
	return ECORE_CALLBACK_PASS_ON;
}

bool app_create(void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_create");
	return true;
}

void app_control(app_control_h app_control, void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_control");
	init_db();
	ecore_timer_add(60*15, timeout_func, NULL);
}

void app_terminate(void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "app_terminate");
	return;
}

int main(int argc, char *argv[]) {
	int ret;

	service_app_lifecycle_callback_s event_callback = { 0, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.app_control = app_control;

	ret = service_app_main(argc, argv, &event_callback, NULL);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "main() failed. err = %d", ret);

	return ret;
}
