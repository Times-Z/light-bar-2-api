#include "nvs.h"

static const char *TAG = "NVS";

/// @brief  Initialize the NVS flash memory (non volatile storage)
/// @param void
/// @return bool : true if the NVS flash memory is initialized, false otherwise
bool nvs_init(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS error detected, flash erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error on erasing : %s", esp_err_to_name(ret));
            return false;
        }

        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error on init nvs : %s", esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(TAG, "NVS initialized");
    return true;
}

/// @brief  Check the space available in the NVS flash memory
/// @param void
/// @return void
void check_nvs_space() {
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get NVS stats (%s)", esp_err_to_name(err));
        return;
    }

    if (nvs_stats.used_entries > (nvs_stats.total_entries * 0.8)) {
        ESP_LOGW(TAG, "NVS memory is almost full, used entries = (%d), total entries = (%d)", nvs_stats.used_entries,
                 nvs_stats.total_entries);
    } else {
        ESP_LOGI(TAG, "NVS memory used entries = (%d), total entries = (%d)", nvs_stats.used_entries,
                 nvs_stats.total_entries);
    }
}

/// @brief save wifi credentials in the nvs partition
/// @param ssid wifi ssid
/// @param password wifi password
/// @return bool true if saved, false otherwise
bool nvs_save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    err |= nvs_set_str(handle, "ssid", ssid);
    err |= nvs_set_str(handle, "password", password);
    err |= nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

/// @brief Load Wi-Fi credentials from NVS
/// @param ssid_out Pointer to the buffer where the SSID will be stored
/// @param ssid_size Size of the SSID buffer (should be at least 33 bytes)
/// @param pass_out Pointer to the buffer where the password will be stored
/// @param pass_size Size of the password buffer (should be at least 65 bytes)
/// @return bool true if both SSID and password are successfully retrieved, false otherwise
bool nvs_load_wifi_credentials(char *ssid_out, size_t ssid_size, char *pass_out, size_t pass_size) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    err |= nvs_get_str(handle, "ssid", ssid_out, &ssid_size);
    err |= nvs_get_str(handle, "password", pass_out, &pass_size);
    nvs_close(handle);
    return err == ESP_OK;
}

/// @brief Clean Wi-Fi creds on nvs
/// @param  void
/// @return bool true on success false otherwise
bool nvs_clear_wifi_credentials(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_erase_key(handle, "ssid");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase SSID key: %s", esp_err_to_name(err));
    }

    err = nvs_erase_key(handle, "password");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase Password key: %s", esp_err_to_name(err));
    }

    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Wi-Fi credentials cleared from NVS");
    return true;
}

/// @brief save ntp domain
/// @param char ntp_domain the ntp domain
/// @return bool true if saved, false otherwise
bool nvs_save_ntp_information(const char *ntp_domain) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("ntp", NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    err |= nvs_set_str(handle, "domain", ntp_domain);
    err |= nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

/// @brief Load ntp domain from NVS
/// @param domain_out Pointer to the buffer where the domain will be stored
/// @param domain_size pointer size
/// @return bool true if ntp domain can be received, false otherwhise
bool nvs_load_ntp_information(char *domain_out, size_t domain_size) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("ntp", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    err |= nvs_get_str(handle, "domain", domain_out, &domain_size);
    nvs_close(handle);
    return err == ESP_OK;
}