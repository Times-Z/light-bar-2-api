#include "webserver.h"
#include "wifi.h"
#include "helper/json.h"
#include "time_sync.h"

esp_err_t status_handler(httpd_req_t* req);
esp_err_t wifi_scan_handler(httpd_req_t* req);
esp_err_t wifi_connect_handler(httpd_req_t* req);
esp_err_t ntp_set_handler(httpd_req_t* req);
esp_err_t logs_handler(httpd_req_t* req);
esp_err_t logs_clear_handler(httpd_req_t* req);
esp_err_t nrf24_scan_handler(httpd_req_t* req);
esp_err_t xiaomi_set_id_handler(httpd_req_t* req);
esp_err_t xiaomi_get_id_handler(httpd_req_t* req);
esp_err_t xiaomi_power_toggle_handler(httpd_req_t* req);
