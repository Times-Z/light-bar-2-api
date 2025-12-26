#include "spiffs_storage.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <esp_vfs_fat.h>

static const char* SPIFFS_TAG = "SPIFFS";
const char* SPIFFS_MOUNTPOINT = "/spiffs";

/// @brief Initialize the SPIFFS partition
/// @param void
/// @return void
void spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_MOUNTPOINT, .partition_label = NULL, .max_files = 5, .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(SPIFFS_TAG, "SPIFFS partition mounted");
    }
}

/// @brief Check the space used in the SPIFFS partition
/// @param void
/// @return void
void spiffs_check_space(void) {
    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);

    ESP_LOGI(SPIFFS_TAG, "SPIFFS Info :");
    ESP_LOGI(SPIFFS_TAG, "Total size: %d KB", total / 1024);
    ESP_LOGI(SPIFFS_TAG, "Free size: %d KB", (total - used) / 1024);

    if (used > total * 0.8) {
        ESP_LOGW(SPIFFS_TAG, "SPIFFS memory is almost full");
    }
}

/// @brief Unmount the SPIFFS partition
/// @param void
/// @return void
void spiffs_deinit(void) {
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(SPIFFS_TAG, "SPIFFS unmounted");
}
