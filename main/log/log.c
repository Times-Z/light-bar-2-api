#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "storage.h"

/// @brief Get the log file path
/// @param void
/// @return const char * : The log file path
static const char *get_log_file_path(void) {
    static char log_file_path[128] = {0};

    if (log_file_path[0] == '\0') {
        snprintf(log_file_path, sizeof(log_file_path), "%s/%s/%s.log", SD_MOUNT_POINT, APP_NAME, APP_VERSION);
    }

    if (!storage_create_directories(log_file_path)) {
        ESP_LOGE("LOG", "Failed to create directories for path: %s", log_file_path);
        return false;
    }

    return log_file_path;
}

/// @brief Translate the log level to a string
/// @param level The log level
/// @return const char * : The log level as a string
static const char *log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG:
            return "DEBUG";
        case LOG_INFO:
            return "INFO";
        case LOG_WARNING:
            return "WARNING";
        case LOG_ERROR:
            return "ERROR";
        case LOG_CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

/// @brief Log a message to the log file
/// @param level The log level
/// @param message The message to log
/// @return void
void log_message(LogLevel level, const char *message) {
    const char *log_path = get_log_file_path();

    FILE *log_file = fopen(log_path, "a");
    if (!log_file) {
        ESP_LOGE("LOG", "Failed to open log file %s", log_path);
        return;
    }

    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] %s: %s\n", time_str, log_level_to_string(level), message);
    fclose(log_file);
}
