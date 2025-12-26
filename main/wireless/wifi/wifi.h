#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <lwip/ip4_addr.h>
#include <netdb.h>
#include <arpa/inet.h>

// BLE disabled - not needed for this project
// #include "../ble/ble.h"
// Captive portal disabled - not present
// #include "../captive_portal/captive_portal.h"
#include "../wireless.h"
#include "../storage/nvs/nvs.h"

extern bool should_save_credentials;
extern esp_netif_t *ap_netif;
extern esp_netif_t *sta_netif;

void wifi_init(void);
void wifi_start_ap(void);
esp_err_t wifi_scan_networks(wifi_ap_record_t **results, uint16_t *ap_count);
uint16_t wifi_get_scan_results(wifi_ap_record_t **results);
bool wifi_start_sta(const char *ssid, const char *password);
const char *wifi_resolve_domain(char *domain_name);
const char *wifi_get_current_ip_str(void);
const char *wifi_get_current_dns_str(void);
