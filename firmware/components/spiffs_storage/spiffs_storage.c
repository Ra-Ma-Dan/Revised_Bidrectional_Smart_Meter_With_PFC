#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiffs_storage.h"

#include "meter_http_server.h"


#define SPIFFS_BASE_PATH "/spiffs"


static const char *TAG = "ENERGY_LOGGER";

int LOG_INTERVAL_MINUTES = 6; // 6 minutes = exactly 10 data logs per hour
int last_logged_day = -1;
int last_logged_minute = -1;

esp_err_t init_energy_logger_storage(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE_PATH,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS Mount Failed");
        return ret;
    }
    return ESP_OK;
}

esp_err_t start_new_logging_day(int day_of_month) {
    if (day_of_month < 1 || day_of_month > 31) return ESP_ERR_INVALID_ARG;
    
    char file_path[32];
    snprintf(file_path, sizeof(file_path), "%s/day_%02d.csv", SPIFFS_BASE_PATH, day_of_month);
    
    // WNNORTE MODE ("w") obliterates last month's data completely on this calendar day block
    FILE *f = fopen(file_path, "w");
    if (f == NULL) return ESP_FAIL;
    
    // Write a clean high-resolution comma-separated header layout
    fprintf(f, "Timestamp,Voltage(V),Current(A),Power(W),Energy(kWh),PF,Credit(NGN)\n");
    fclose(f);
    
    ESP_LOGI(TAG, "Cleared last month's history. Started fresh log file for Day: %02d", day_of_month);
    return ESP_OK;
}

esp_err_t append_hourly_log_point(int day_of_month, energy_log_point_t point) {
    if (day_of_month < 1 || day_of_month > 31) return ESP_ERR_INVALID_ARG;
    
    char file_path[32];
    snprintf(file_path, sizeof(file_path), "%s/day_%02d.csv", SPIFFS_BASE_PATH, day_of_month);
    
    // APPEND MODE ("a") pushes text smoothly to the bottom of the active file tracking grid
    FILE *f = fopen(file_path, "a");
    if (f == NULL) {
        // Fallback safety safety if user manual resetting dropped the system file unexpectedly
        start_new_logging_day(day_of_month);
        f = fopen(file_path, "a");
        if (f == NULL) return ESP_FAIL;
    }
    
    fprintf(f, "%02d:%02d,%.1f,%.3f,%.1f,%.3f,%.2f,%.2f\n",
            point.hour, point.minute, point.voltage, point.current,
            point.active_power, point.cumulative_kwh, point.power_factor, point.remaining_credit);
            
    fclose(f);
    return ESP_OK;
}

void run_system_logger_telemetry(int current_day, int current_hour, int current_minute) {
    // 1. Check if the calendar day rolled over to a new date context
    if (current_day != last_logged_day) {
        start_new_logging_day(current_day);
        last_logged_day = current_day;
    }

    // 2. Fire logs at precise clean sub-hourly interval segments
    if (current_minute % LOG_INTERVAL_MINUTES == 0 && current_minute != last_logged_minute) {
        energy_log_point_t live_snapshot;
        
        live_snapshot.hour             = current_hour;
        live_snapshot.minute           = current_minute;
        live_snapshot.voltage          = live_pfc_data.voltage;
        live_snapshot.current          = live_pfc_data.current;
        live_snapshot.active_power     = live_pfc_data.power;
        live_snapshot.cumulative_kwh   = (live_pfc_data.energy / 1000.0f);
        live_snapshot.power_factor     = live_pfc_data.power_factor;
        live_snapshot.remaining_credit = credit_balance;

        // Commit line down to storage partition block!
        append_hourly_log_point(current_day, live_snapshot);
        last_logged_minute = current_minute;
    }
}


esp_err_t read_day_log_payload(int day_of_month, char *output_buffer, size_t max_size) {
    if (day_of_month < 1 || day_of_month > 31) return ESP_ERR_INVALID_ARG;
    
    char file_path[32];
    snprintf(file_path, sizeof(file_path), "%s/day_%02d.csv", SPIFFS_BASE_PATH, day_of_month);
    
    struct stat st;
    if (stat(file_path, &st) != 0) {
        snprintf(output_buffer, max_size, "No log records found for today.");
        return ESP_OK;
    }
    
    FILE *f = fopen(file_path, "r");
    if (f == NULL) return ESP_FAIL;
    
    size_t bytes_read = fread(output_buffer, 1, max_size - 1, f);
    output_buffer[bytes_read] = '\0';
    
    fclose(f);
    return ESP_OK;
}


void init_system_clock_sntp(void) {
    ESP_LOGI("TIME_SYNC", "Initializing SNTP Client Engine...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
    // Use Africa/Lagos pool or global public servers
    esp_sntp_setservername(0, "pool.ntp.org"); 
    esp_sntp_init();

    // Set time zone to West Africa Time (WAT) - Nigeria is UTC+1
    setenv("TZ", "WAT-1", 1);
    tzset();
}


void energy_logger_background_task(void *pvParameters) {
    ESP_LOGI("LOGGER_TASK", "High-Resolution Logging Task Started.");
    
    while (1) {
        time_t now;
        struct tm timeinfo;
        
        // 1. Extract the current raw time snapshot from the ESP32 internal core clock
        time(&now);
        localtime_r(&now, &timeinfo);

        // 2. Safety filter: Ensure the clock has synced with NTP before writing files.
        // If the year is still 1970, the clock hasn't updated yet!
        if (timeinfo.tm_year >= (2026 - 1900)) { 
            
            // Extract exact calendar tracking values matching our required parameters:
            int current_day    = timeinfo.tm_mday; // Day of month (1 to 31)
            int current_hour   = timeinfo.tm_hour; // Hour of day (0 to 23)
            int current_minute = timeinfo.tm_min;  // Minute of hour (0 to 59)

            // 3. Call your function and feed it the live variables directly!
            run_system_logger_telemetry(current_day, current_hour, current_minute);
        } else {
            ESP_LOGW("LOGGER_TASK", "Waiting for stable NTP time synchronization over Wi-Fi...");
        }

        // 4. Delay the loop execution for 10 seconds. 
        // This stops your task from consuming CPU while it waits for the next minute boundary.
        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}
