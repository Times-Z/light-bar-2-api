#pragma once

#include "wifi/wifi.h"
#include "ble/ble.h"
#include "lwip/ip4_addr.h"

extern bool Scan_finish;

bool wireless_Init(void);
char* ip_to_str(const esp_ip4_addr_t* ip);
const char* authmode_to_str(wifi_auth_mode_t authmode);
