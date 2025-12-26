#include <time_sync.h>

static const char *TAG = "TIME_SYNC";
static int NTP_SYNC_TIMEOUT = 30;
static bool s_time_synced = false;

static void ntp_sync_notification_cb(struct timeval *tv) {
    s_time_synced = true;
    ESP_LOGI(TAG, "NTP sync callback fired");
}

/// @brief init system datetime
/// @param void
/// @return void
void time_init(void) {
    char ntp_domain[33] = {0};

    if (nvs_load_ntp_information(ntp_domain, sizeof(ntp_domain)) && strlen(ntp_domain) > 0) {
        ESP_LOGI(TAG, "Found NTP domain in NVS: %s", ntp_domain);

        if (time_sync_with_ntp(ntp_domain)) {
            ESP_LOGI(TAG, "Time sync with NTP server successful");
            return;
        } else {
            ESP_LOGW(TAG, "Failed to sync with NTP, using build time fallback");
        }
    } else {
        ESP_LOGI(TAG, "No NTP server configured, using build time fallback");
    }

    time_sync_from_build();
}

/// @brief Sync system datetime with build informations
/// @param void
/// @return void
void time_sync_from_build(void) {
    struct tm tm = {0};
    char month[4];
    int day, year, hour, minute, second;
    sscanf(__DATE__, "%3s %d %d", month, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    int month_num = 0;
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            month_num = i;
            break;
        }
    }

    tm.tm_year = year - 1900;
    tm.tm_mon = month_num;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;

    time_t t = mktime(&tm);
    if (t == -1) {
        ESP_LOGE(TAG, "Error converting build date to time_t");
        return;
    }

    struct timeval tv = {.tv_sec = t, .tv_usec = 0};

    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "Failed to set system time");
    } else {
        char datetime_str[26];
        strncpy(datetime_str, asctime(&tm), sizeof(datetime_str) - 1);
        datetime_str[strcspn(datetime_str, "\n")] = '\0';
        ESP_LOGI(TAG, "Initialized system time from build time: %s", datetime_str);
    }
}

/// @brief sync time with ntp server
/// @param char ntp_domain the ntp domain
/// @return bool true if change is ok, false otherwise
bool time_sync_with_ntp(const char *ntp_domain) {
    if (ntp_domain == NULL || strlen(ntp_domain) == 0) {
        ESP_LOGE(TAG, "NTP domain is empty or NULL.");
        return false;
    }

    s_time_synced = false;
    esp_sntp_stop();

    ESP_LOGI(TAG, "Starting SNTP sync with: %s", ntp_domain);

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, ntp_domain);
    sntp_set_time_sync_notification_cb(ntp_sync_notification_cb);
    esp_sntp_init();

    for (int retry = 0; retry < NTP_SYNC_TIMEOUT; retry++) {
        if (s_time_synced || sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);

            char datetime_str[26];
            strncpy(datetime_str, asctime(&timeinfo), sizeof(datetime_str) - 1);
            datetime_str[strcspn(datetime_str, "\n")] = '\0';

            nvs_save_ntp_information(ntp_domain);
            ESP_LOGI(TAG, "Time synced successfully : %s", datetime_str);
            return true;
        }

        ESP_LOGW(TAG, "Waiting for NTP sync... (%d/%d)", retry + 1, NTP_SYNC_TIMEOUT);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGE(TAG, "NTP sync failed after timeout");
    return false;
}
