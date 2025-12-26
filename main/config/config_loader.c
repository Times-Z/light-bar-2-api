#include "config_loader.h"

#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <cJSON.h>

static const char* TAG = "CONFIG_LOADER";
static const char* CONFIG_PATH = "/config/config.json";

/// @brief Load Wi-Fi credentials from SPIFFS config partition file /config/config.json
/// @param ssid_out ssid output buffer
/// @param ssid_size ssid output buffer size
/// @param pass_out password output buffer
/// @param pass_size password output buffer size
/// @return true if both SSID and password were successfully parsed, false otherwise
bool config_load_wifi_credentials(char* ssid_out, size_t ssid_size, char* pass_out, size_t pass_size) {
    if (!ssid_out || !pass_out || ssid_size == 0 || pass_size == 0) {
        ESP_LOGE(TAG, "Invalid buffers for Wi-Fi credentials");
        return false;
    }

    FILE* f = fopen(CONFIG_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "Config file not found at %s", CONFIG_PATH);
        return false;
    }

    char buf[1024];
    size_t read_len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[read_len] = '\0';

    cJSON* root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON config");

        return false;
    }

    const cJSON* ssid = cJSON_GetObjectItemCaseSensitive(root, "wifi_ssid");
    const cJSON* password = cJSON_GetObjectItemCaseSensitive(root, "wifi_password");

    if (cJSON_IsString(ssid) && (ssid->valuestring != NULL) && strlen(ssid->valuestring) > 0 &&
        cJSON_IsString(password) && (password->valuestring != NULL)) {
        strlcpy(ssid_out, ssid->valuestring, ssid_size);
        strlcpy(pass_out, password->valuestring, pass_size);
        ESP_LOGI(TAG, "Loaded Wi-Fi credentials from config.json (SSID: %s)", ssid_out);

        cJSON_Delete(root);
        return true;
    } else {
        ESP_LOGW(TAG, "Missing or invalid wifi_ssid/wifi_password in config.json");
        cJSON_Delete(root);

        return false;
    }
}
