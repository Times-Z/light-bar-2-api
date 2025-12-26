#include "wireless.h"
#include "wifi/wifi.h"
#include "ble/ble.h"

bool WiFi_Scan_Finish;

bool wireless_Init(void) {
    wifi_init();
    // ble_init();  // about 55kb ram usage atm

    return true;
}

/// @brief Transform esp_ip4_addr_t to string
/// @param esp_ip4_addr_t
/// @return char stringify ip
char *ip_to_str(const esp_ip4_addr_t *ip) {
    static char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
    return ip_str;
}

/// @brief Return wifi_auth_mode_t to string
/// @param wifi_auth_mode_t authmode
/// @return char stringify authmode
const char *authmode_to_str(wifi_auth_mode_t authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3-PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WAPI-PSK";
        case WIFI_AUTH_OWE:
            return "OWE";
        case WIFI_AUTH_WPA3_ENT_192:
            return "WPA3-Enterprise-192";
        default:
            return "Unknown method";
    }
}
