#pragma once

#include <esp_err.h>
#include <stdint.h>

/// Result from Xiaomi scan
typedef struct {
    uint32_t found_count;
    uint8_t pattern_0[8];  // Most common pattern
    uint8_t pattern_1[8];  // Second pattern
    uint8_t pattern_2[8];  // Third pattern
    uint32_t last_scan_time;
    uint32_t remote_id;     // Decoded 3-byte Xiaomi remote ID
    uint8_t id_found;       // 1 if remote_id is valid, 0 otherwise
    uint8_t commands_mask;  // Bitmask of seen commands: bit0 on/off, 1 cooler, 2 warmer, 3 higher, 4 lower, 5 reset
} xiaomi_scan_result_t;

esp_err_t nrf24_check_connection(void);
esp_err_t nrf24_scan_xiaomi(uint32_t duration_ms);
const xiaomi_scan_result_t* nrf24_get_last_scan_result(void);
