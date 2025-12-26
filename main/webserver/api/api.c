#include "api.h"
#include "v1/v1.h"

static const char* TAG = "API";
extern bool should_save_credentials;
extern esp_netif_t* ap_netif;
extern esp_netif_t* sta_netif;

static esp_err_t preflight_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Handling OPTIONS request for CORS preflight");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return httpd_resp_send(req, NULL, 0);
}

/// @brief Register API v1 endpoints
/// @param server httpd_handle_t
/// @return void
void register_api_v1_endpoints(httpd_handle_t server) {
    httpd_uri_t status_uri = {
        .uri = "/api/v1/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t wifi_scan_uri = {
        .uri = "/api/v1/wifi/scan",
        .method = HTTP_GET,
        .handler = wifi_scan_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t wifi_connect_uri = {
        .uri = "/api/v1/wifi/connect",
        .method = HTTP_POST,
        .handler = wifi_connect_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t ntp_sync_uri = {
        .uri = "/api/v1/ntp/set",
        .method = HTTP_POST,
        .handler = ntp_set_handler,
        .user_ctx = NULL,
    };

    httpd_uri_t preflight_uri = {
        .uri = "/api/v1/*",
        .method = HTTP_OPTIONS,
        .handler = preflight_handler,
        .user_ctx = NULL,
    };

    ESP_LOGI(TAG, "Registering API v1 endpoints");
    httpd_register_uri_handler(server, &status_uri);
    httpd_register_uri_handler(server, &wifi_scan_uri);
    httpd_register_uri_handler(server, &wifi_connect_uri);
    httpd_register_uri_handler(server, &ntp_sync_uri);
    httpd_register_uri_handler(server, &preflight_uri);
}
