#include "ble.h"

uint16_t ble_num = 0;
bool ble_scan_finish = false;

static discovered_device_t discovered_devices[MAX_DISCOVERED_DEVICES];
static size_t num_discovered_devices = 0;

static const char *TAG = "BLE_SCAN";

/// @brief Check if a BLE device has already been discovered based on its address
/// @param addr Pointer to the 6-byte BLE address
/// @return true if the device is already in the list, false otherwise
static bool is_device_discovered(const uint8_t *addr) {
    for (size_t i = 0; i < num_discovered_devices; i++) {
        if (memcmp(discovered_devices[i].address, addr, 6) == 0) {
            return true;
        }
    }
    return false;
}

/// @brief Add a BLE device to the discovered devices list
/// @param addr Pointer to the 6-byte BLE address of the new device
/// @return void
static void add_device_to_list(const uint8_t *addr) {
    if (num_discovered_devices < MAX_DISCOVERED_DEVICES) {
        memcpy(discovered_devices[num_discovered_devices].address, addr, 6);
        discovered_devices[num_discovered_devices].is_valid = true;
        num_discovered_devices++;
    }
}

/// @brief NimBLE sync callback. Called when BLE host stack is ready
/// @param void
/// @return void
static void ble_app_on_sync(void) {
    uint8_t addr_type;
    int rc = ble_hs_id_infer_auto(0, &addr_type);

    if (rc != 0) {
        ESP_LOGE("BLE", "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }

    ble_scan();
}

/// @brief BLE GAP event handler. Handles discovered devices and scan completion
/// @param event Pointer to the BLE GAP event structure
/// @param arg User-defined argument (unused)
/// @return 0 on success
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            ESP_LOGI(TAG, "Device discovered");
            struct ble_hs_adv_fields fields;
            int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) {
                ESP_LOGW(TAG, "Failed to parse advertisement data");
            }

            const uint8_t *addr = event->disc.addr.val;
            if (!is_device_discovered(addr)) {
                add_device_to_list(addr);
                ble_num++;
                char addr_str[18];
                snprintf(addr_str, sizeof(addr_str), "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2],
                         addr[3], addr[4], addr[5]);
                ESP_LOGI(TAG, "New device: %s", addr_str);
            }
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:
            ESP_LOGI(TAG, "Scan complete");
            ble_scan_finish = true;
            return 0;

        default:
            return 0;
    }
}

/// @brief Start BLE scan with predefined parameters.
///        This function triggers passive scanning and handles discovered devices via callbacks.
void ble_scan(void) {
    struct ble_gap_disc_params scan_params = {
        .passive = 1,
        .filter_duplicates = 1,
        .limited = 0,
    };

    int rc = ble_gap_disc(0, SCAN_DURATION * 1000, &scan_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error initiating GAP discovery procedure: %d", rc);
    }
}

/// @brief NimBLE host task. Must be run in its own FreeRTOS task context.
/// @param param Unused parameter.
void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/// @brief Initialize the NimBLE stack and start the host task.
///        Sets up BLE configuration and prepares the system for scanning.
void ble_init(void) {
    nimble_port_init();

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    nimble_port_freertos_init(ble_host_task);
}
