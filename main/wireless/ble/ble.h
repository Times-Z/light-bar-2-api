#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <esp_mac.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <host/util/util.h>
#include <nimble/ble.h>

#define SCAN_DURATION 5
#define MAX_DISCOVERED_DEVICES 100

typedef struct {
    uint8_t address[6];
    bool is_valid;
} discovered_device_t;

extern uint16_t ble_num;
extern bool ble_scan_finish;

void ble_init(void);
void ble_scan(void);
