// thanks to https://developer.tizen.org/ko/forums/native-application-development/complete-tutorial-sqlite-database-crud-operation-and-data-access-tizen-native-application

#include <batterymonitoringservice.h>
#include <string.h>
#include <dlog.h>
#include <database.h>
#include <storage.h>
#include <sqlite3.h>
#include <storage.h>
#include <Elementary.h>
#include <service_app.h>

sqlite3 *database; // DB instance
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
