#include "json.h"
#include <cJSON.h>
#include <esp_log.h>

#define TAG "JSON"

/// @brief Generate a safe json with cJSON
/// @param count number of json entries
/// @param json_entry_t entries
/// @return char the json string
char *build_json_safe(int count, const json_entry_t *entries) {
    if (!entries || count <= 0) {
        ESP_LOGE(TAG, "Invalid JSON input: entries is NULL or count <= 0");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create cJSON root");
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        const char *key = entries[i].key;
        json_type_t type = entries[i].type;
        const void *value = entries[i].value;

        if (!key) {
            ESP_LOGW(TAG, "Entry %d skipped: key is NULL", i);
            continue;
        }

        switch (type) {
            case JSON_TYPE_STRING:
                ESP_LOGD(TAG, "Adding string: %s = \"%s\"", key, value ? (const char *)value : "");
                cJSON_AddStringToObject(root, key, value ? (const char *)value : "");
                break;

            case JSON_TYPE_NUMBER:
                if (!value) {
                    ESP_LOGW(TAG, "Entry %s skipped: number value is NULL", key);
                    break;
                }
                ESP_LOGD(TAG, "Adding number: %s = %d", key, *(int *)value);
                cJSON_AddNumberToObject(root, key, *(int *)value);
                break;

            case JSON_TYPE_BOOL:
                if (!value) {
                    ESP_LOGW(TAG, "Entry %s skipped: bool value is NULL", key);
                    break;
                }
                ESP_LOGD(TAG, "Adding bool: %s = %s", key, (*(int *)value) ? "true" : "false");
                cJSON_AddBoolToObject(root, key, *(int *)value);
                break;

            case JSON_TYPE_RAW:
                if (!value) {
                    ESP_LOGW(TAG, "Entry %s skipped: raw value is NULL", key);
                    cJSON_AddStringToObject(root, key, "NULL_RAW");
                    break;
                }
                ESP_LOGD(TAG, "Adding RAW: %s", key);
                cJSON *parsed = cJSON_Parse((const char *)value);
                if (parsed) {
                    cJSON_AddItemToObject(root, key, parsed);
                } else {
                    ESP_LOGW(TAG, "Failed to parse RAW JSON: %s", (const char *)value);
                    cJSON_AddStringToObject(root, key, "INVALID_JSON_RAW");
                }
                break;

            default:
                ESP_LOGW(TAG, "Unknown type for key %s", key);
                break;
        }
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        return NULL;
    }

    ESP_LOGI(TAG, "Generated JSON : %s", json_str);
    return json_str;
}
