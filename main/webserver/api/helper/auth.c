#include "auth.h"
#include "json.h"

#include <string.h>
#include <esp_log.h>

static const char* TAG = "AUTH";
// Default API key - can be overridden in config.json
static const char* DEFAULT_API_KEY = "your_api_key_here";

// Loaded at runtime from config.json
static char loaded_api_key[129] = {0};
static bool key_initialized = false;

/// @brief Implements helper routines for API authentication
/// @param api_key API key to initialize
/// @return Status code indicating whether authentication succeeded or failed
void auth_init_from_config(const char* api_key) {
    if (api_key && strlen(api_key) > 0 && strlen(api_key) < sizeof(loaded_api_key)) {
        strlcpy(loaded_api_key, api_key, sizeof(loaded_api_key));
        key_initialized = true;
        ESP_LOGI(TAG, "API key initialized from config");
    } else {
        strlcpy(loaded_api_key, DEFAULT_API_KEY, sizeof(loaded_api_key));
        key_initialized = true;
        ESP_LOGW(TAG, "Using default API key");
    }
}

bool auth_validate_api_key(httpd_req_t* req) {
    if (!key_initialized) {
        auth_init_from_config(NULL);
    }

    char api_key_header[129] = {0};
    size_t hdr_len = httpd_req_get_hdr_value_len(req, "X-API-Key");
    if (hdr_len > 0 && hdr_len < sizeof(api_key_header)) {
        esp_err_t err = httpd_req_get_hdr_value_str(req, "X-API-Key", api_key_header, sizeof(api_key_header));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read X-API-Key header (%d)", err);
            return false;
        }
    } else if (hdr_len >= sizeof(api_key_header)) {
        ESP_LOGW(TAG, "X-API-Key header too long (%u)", (unsigned)hdr_len);
        return false;
    }

    bool valid = (strlen(api_key_header) > 0 && strcmp(api_key_header, loaded_api_key) == 0);

    if (!valid) {
        ESP_LOGW(TAG, "Invalid or missing API key");
    }

    return valid;
}

esp_err_t auth_send_unauthorized(httpd_req_t* req, const char* message) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    json_entry_t error_json[] = {
        {"success", JSON_TYPE_BOOL, &(int){0}},
        {"message", JSON_TYPE_STRING, (char*)message},
    };

    char* json_response = build_json_safe(JSON_ARRAY_SIZE(error_json), error_json);
    esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);

    return res;
}
