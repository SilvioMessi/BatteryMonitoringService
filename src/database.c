// for sqlite 3 thanks to https://developer.tizen.org/ko/forums/native-application-development/complete-tutorial-sqlite-database-crud-operation-and-data-access-tizen-native-application
// for data control thanks to https://developer.tizen.org/ko/development/guides/native-application/application-management/application-data-exchange/data-control

#include <batterymonitoringservice.h>
#include <string.h>
#include <dlog.h>
#include <database.h>
#include <data_control.h>
#include <storage.h>
#include <sqlite3.h>
#include <storage.h>
#include <Elementary.h>
#include <service_app.h>

static sqlite3 *database; // DB instance
data_control_provider_sql_cb *sql_callback; // data control callbacks
int n_samples = 0;

int opendb() {
	// create DB file path
	/* TODO: find correct DB location
	 * files stored in the application data folder (app_get_data_path()) are delete each time the application is re-installed
	 * is this fine for the production version of the application ?*/
	char db_path[256] = { "\0" };
	snprintf(db_path, sizeof(db_path), "%s%s", app_get_data_path(), DB_NAME);

	// open/create DB connection
	return sqlite3_open_v2(db_path, &database,
	SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
}

int execute_query(char *sql, int (*callback)(void*, int, char**, char**)) {
	int error_code;
	char *error_message;

	// try to open DB connection
	error_code = opendb();
	if (error_code != SQLITE_OK) {
		dlog_print(DLOG_ERROR, LOG_TAG, "cannot open DB connection: %s",
				sqlite3_errmsg(database));
		return error_code;
	}

	// execute query
	error_code = sqlite3_exec(database, sql, callback, 0, &error_message);
	if (error_code != SQLITE_OK) {
		dlog_print(DLOG_ERROR, LOG_TAG, "query failed: %s", error_message);
		sqlite3_free(error_message);
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "query executed successfully");
	}

	// close the DB
	sqlite3_close(database);
	return error_code;
}

int init_db() {
	// create table query
	char *sql = "CREATE TABLE IF NOT EXISTS "
	TABLE_NAME" ("
	COL_TIMESTAMP" TEXT NOT NULL, "
	COL_BATTERY_PERCENTAGE" INTEGER NOT NULL, "
	COL_BATTERY_IS_CHARGING" INTEGER NOT NULL, "
	COL_ID" INTEGER PRIMARY KEY AUTOINCREMENT);";
	dlog_print(DLOG_DEBUG, LOG_TAG, "create table query: %s", sql);
	return execute_query(sql, NULL);
}

int insert_sample(char *time, int battery_percentage, int battery_is_charging) {
	char sql[QUERYLEN] = { "\0" };

	// prepare query for INSERT operation
	snprintf(sql, QUERYLEN,
			"INSERT INTO "TABLE_NAME" VALUES(\'%s\',%d, %d, NULL);", time,
			battery_percentage, battery_is_charging);
	dlog_print(DLOG_DEBUG, LOG_TAG, "insert sample query: %s", sql);
	return execute_query(sql, NULL);
}

static int num_of_samples_cb(void *data, int argc, char **argv, char **col) {
	n_samples = atoi(argv[0]);
	return 0;
}

int num_of_samples(int *num_of_samples) {
	int error_code;

	// create SELECT COUNT(*) query
	char *sql = "SELECT COUNT(*) FROM "TABLE_NAME";";
	dlog_print(DLOG_DEBUG, LOG_TAG, "select count(*) query: %s", sql);
	error_code = execute_query(sql, num_of_samples_cb);

	// return value(s) got from the query
	if (error_code != SQLITE_OK) {
		*num_of_samples = -1;
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "number of samples: %d", n_samples);
		*num_of_samples = n_samples;
	}
	return error_code;
}

void select_request_cb(int request_id, data_control_h provider,
		const char **column_list, int column_count, const char *where,
		const char *order, void *user_data) {
	sqlite3_stmt* sql_stmt = NULL;

	// try to open DB connection
	int error_code = opendb();
	if (error_code != SQLITE_OK) {
		data_control_provider_send_error(request_id, sqlite3_errmsg(database));
		return;
	}

	// execute query
	char* command = data_control_provider_create_select_statement(provider,
			column_list, column_count, where, order);
	error_code = sqlite3_prepare_v2(database, command, strlen(command),
			&sql_stmt, NULL);
	if (error_code != SQLITE_OK) {
		data_control_provider_send_error(request_id, sqlite3_errmsg(database));
	} else {
		// return data
		error_code = data_control_provider_send_select_result(request_id,
				(void *) sql_stmt);
		if (error_code != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"select_send_result failed with error: %d", error_code);
		dlog_print(DLOG_INFO, LOG_TAG,
				"select_request_cb send back result successfully");
	}

	// close the DB
	sqlite3_close(database);
	sqlite3_finalize(sql_stmt);
	free(command);
	return;
}

void initialize_datacontrol_provider() {
	dlog_print(DLOG_INFO, LOG_TAG, "initialize_datacontrol_provider");
	int result = init_db();
	if (result != SQLITE_OK)
		return;

	sql_callback = (data_control_provider_sql_cb *) malloc(
			sizeof(data_control_provider_sql_cb));
	sql_callback->select_cb = select_request_cb;
	result = data_control_provider_sql_register_cb(sql_callback, NULL);
	if (result != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG,
				"data_control_provider_sql_register_cb failed with error: %d", result);
	else
		dlog_print(DLOG_INFO, LOG_TAG, "provider SQL register success");
}
