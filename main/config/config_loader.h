#pragma once

#include <stddef.h>
#include <stdbool.h>

bool config_load_wifi_credentials(char* ssid_out, size_t ssid_size, char* pass_out, size_t pass_size);
bool config_load_api_key(char* api_key_out, size_t api_key_size);
