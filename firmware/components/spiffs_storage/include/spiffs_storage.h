#ifndef SPIFFS_STORAGE_H
#define SPIFFS_STORAGE_H

#include "esp_err.h"

#define SPIFFS_BASE_PATH "/spiffs"

extern int LOG_INTERVAL_MINUTES; // 6 minutes = exactly 10 data logs per hour

// High-resolution data point structure
typedef struct {
    int hour;
    int minute;
    float voltage;
    float current;
    float active_power;
    float cumulative_kwh;
    float power_factor;
    float remaining_credit;
} energy_log_point_t;

// Initialize SPIFFS Partition
esp_err_t init_energy_logger_storage(void);

// Wipes last month's file data and starts a fresh spreadsheet for the new day
esp_err_t start_new_logging_day(int day_of_month);

// Appends a snapshot point to the active day file (Run this 4–10 times per hour)
esp_err_t append_hourly_log_point(int day_of_month, energy_log_point_t point);

// Streams out the entire active day's timeline or logs for download
esp_err_t read_day_log_payload(int day_of_month, char *output_buffer, size_t max_size);

void init_system_clock_sntp(void);
void energy_logger_background_task(void *pvParameters);

#endif // SPIFFS_STORAGE_H
