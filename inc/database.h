#ifndef DATABASE_H_
#define DATABASE_H_

#define DB_NAME "batterymonitoringservice.db"
#define TABLE_NAME "battery_status_samples"
#define COL_ID "ID"
#define COL_TIMESTAMP "TIMESTAMP"
#define COL_BATTERY_PERCENTAGE "BATTERY_PERCENTAGE"
#define COL_BATTERY_IS_CHARGING "BATTERY_IS_CHARGING"
#define QUERYLEN 500 // query max length

int insert_sample(char *time, int battery_percentage, int battery_is_charging);

void initialize_datacontrol_provider();

#endif /* DATABASE_H_ */
