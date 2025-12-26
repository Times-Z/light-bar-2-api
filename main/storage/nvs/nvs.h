#pragma once

#include <esp_log.h>
#include <nvs_flash.h>

bool nvs_init(void);
void check_nvs_space();
bool nvs_save_wifi_credentials(const char *ssid, const char *password);
bool nvs_load_wifi_credentials(char *ssid_out, size_t ssid_size, char *pass_out, size_t pass_size);
bool nvs_clear_wifi_credentials(void);
bool nvs_save_ntp_information(const char *ntp_domain);
bool nvs_load_ntp_information(char *domain_out, size_t domain_size);
