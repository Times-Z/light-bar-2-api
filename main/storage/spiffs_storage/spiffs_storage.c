#include "spiffs_storage.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <esp_spiffs.h>

static const char* SPIFFS_TAG = "SPIFFS";

/// @brief Initializes SPIFFS storage with the specified configuration
/// @param label A descriptive label for the storage instance
/// @param mountpoint The mount point path where SPIFFS will be mounted in the filesystem
/// @param max_files Maximum number of files that can be opened simultaneously
/// @return ESP_OK on success, or an error code indicating the failure reason
static esp_err_t spiffs_register_partition(const char* label, const char* mountpoint, int max_files) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = mountpoint,
        .partition_label = label,
        .max_files = max_files,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_TAG, "Failed to mount or format filesystem for %s", label);
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_TAG, "Failed to find SPIFFS partition %s", label);
        } else {
            ESP_LOGE(SPIFFS_TAG, "Failed to initialize SPIFFS %s (%s)", label, esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(SPIFFS_TAG, "SPIFFS partition %s mounted at %s", label, mountpoint);

    return ret;
}

static void spiffs_log_info(const char* label, const char* name) {
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_TAG, "Failed to get %s partition information (%s)", name, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(SPIFFS_TAG, "%s: total %d KB, free %d KB", name, (int)(total / 1024), (int)((total - used) / 1024));
}

/// @brief Initialize the SPIFFS partition
/// @param void
/// @return void
void spiffs_init(void) {
    esp_err_t config_ret = spiffs_register_partition(CONFIG_PARTITION_LABEL, CONFIG_MOUNTPOINT, 5);
    esp_err_t www_ret = spiffs_register_partition(WWW_PARTITION_LABEL, WWW_MOUNTPOINT, 8);

    if (config_ret == ESP_OK) {
        spiffs_log_info(CONFIG_PARTITION_LABEL, "config");
    }

    if (www_ret == ESP_OK) {
        spiffs_log_info(WWW_PARTITION_LABEL, "www");
    }
}

/// @brief Check the space used in the SPIFFS partition
/// @param void
/// @return void
void spiffs_check_space(void) {
    spiffs_log_info(CONFIG_PARTITION_LABEL, "config");
    spiffs_log_info(WWW_PARTITION_LABEL, "www");
}

/// @brief Unmount the SPIFFS partition
/// @param void
/// @return void
void spiffs_deinit(void) {
    esp_vfs_spiffs_unregister(CONFIG_PARTITION_LABEL);
    esp_vfs_spiffs_unregister(WWW_PARTITION_LABEL);
    ESP_LOGI(SPIFFS_TAG, "SPIFFS partitions unmounted");
}
