#include "api.h"
#include "v1/v1.h"
#include "helper/auth.h"
#include <stdbool.h>

static const char* TAG = "API";
extern bool should_save_credentials;
extern esp_netif_t* ap_netif;
extern esp_netif_t* sta_netif;

typedef struct {
    esp_err_t (*handler)(httpd_req_t* req);
    bool require_auth;
} api_handler_ctx_t;

/// @brief Handles an incoming API request and processes it accordingly
/// @param req The HTTP request object containing headers, method, path, and body
/// @return An HTTP response object with status code and response body
static esp_err_t preflight_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Handling OPTIONS request for CORS preflight");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, X-API-Key");
    return httpd_resp_send(req, NULL, 0);
}

/// @brief Initialize API authentication with key from config
/// @param api_key API key to use
/// @return void
void api_init_auth(const char* api_key) { auth_init_from_config(api_key); }

/// @brief Handles API requests for the webserver
/// @param req Pointer to the HTTP request structure containing request details
/// @return ESP_OK on success, appropriate ESP error code on failure
static esp_err_t api_dispatch(httpd_req_t* req) {
    const api_handler_ctx_t* ctx = (const api_handler_ctx_t*)req->user_ctx;
    if (!ctx || !ctx->handler) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Handler not configured");
    }

    if (ctx->require_auth && !auth_validate_api_key(req)) {
        return auth_send_unauthorized(req, "Missing or invalid X-API-Key header");
    }

    return ctx->handler(req);
}

/// @brief Register API v1 endpoints
/// @param server httpd_handle_t server instance
/// @return void
void register_api_v1_endpoints(httpd_handle_t server) {
    static const api_handler_ctx_t ctx_status = {.handler = status_handler, .require_auth = false};
    static const api_handler_ctx_t ctx_wifi_scan = {.handler = wifi_scan_handler, .require_auth = true};
    static const api_handler_ctx_t ctx_wifi_connect = {.handler = wifi_connect_handler, .require_auth = true};
    static const api_handler_ctx_t ctx_ntp_sync = {.handler = ntp_set_handler, .require_auth = true};
    static const api_handler_ctx_t ctx_logs = {.handler = logs_handler, .require_auth = true};
    static const api_handler_ctx_t ctx_logs_clear = {.handler = logs_clear_handler, .require_auth = true};

    httpd_uri_t status_uri = {
        .uri = "/api/v1/status",
        .method = HTTP_GET,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_status,
    };
    httpd_uri_t wifi_scan_uri = {
        .uri = "/api/v1/wifi/scan",
        .method = HTTP_GET,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_wifi_scan,
    };
    httpd_uri_t wifi_connect_uri = {
        .uri = "/api/v1/wifi/connect",
        .method = HTTP_POST,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_wifi_connect,
    };
    httpd_uri_t ntp_sync_uri = {
        .uri = "/api/v1/ntp/set",
        .method = HTTP_POST,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_ntp_sync,
    };
    httpd_uri_t logs_uri = {
        .uri = "/api/v1/logs",
        .method = HTTP_GET,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_logs,
    };
    httpd_uri_t logs_clear_uri = {
        .uri = "/api/v1/logs/clear",
        .method = HTTP_DELETE,
        .handler = api_dispatch,
        .user_ctx = (void*)&ctx_logs_clear,
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
    httpd_register_uri_handler(server, &logs_uri);
    httpd_register_uri_handler(server, &logs_clear_uri);
    httpd_register_uri_handler(server, &preflight_uri);
}
