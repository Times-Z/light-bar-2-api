#include "webserver.h"
#include "api/api.h"
#include "storage.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "config_loader.h"

static const char* TAG = "HTTP_SERVER";

/// @brief Handles HTTP request and sends a file response with specified content type
/// @param req The HTTP request object containing request details
/// @param file_path The path to the file to be served in response
/// @param content_type The MIME type of the file content (e.g., "text/html", "application/json")
/// @return HTTP status code indicating the result of the request handling operation
static esp_err_t serve_file(httpd_req_t* req, const char* file_path, const char* content_type) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
    }

    httpd_resp_set_type(req, content_type);
    char buffer[512];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        esp_err_t send_status = httpd_resp_send_chunk(req, buffer, bytes_read);
        if (send_status != ESP_OK) {
            fclose(file);
            ESP_LOGE(TAG, "Error sending chunk from file: %s", file_path);
            return send_status;
        }
    }
    fclose(file);
    return httpd_resp_send_chunk(req, NULL, 0);
}

/// @brief Return the content type based on the file extension
/// @param path The file path to serve relative to the web root directory
/// @return The MIME type string corresponding to the file extension
static const char* get_content_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg")) return "image/jpeg";
    return "text/plain";
}

/// @brief Returns the handler for serving static files
/// @param req Pointer to the HTTP request structure containing request details
/// @return HTTP response code indicating success or failure of request processing
static esp_err_t static_file_handler(httpd_req_t* req) {
    char full_path[256];
    strlcpy(full_path, WWW_MOUNTPOINT, sizeof(full_path));

    if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, "/home") == 0 || strcmp(req->uri, "/setup") == 0) {
        strlcat(full_path, "/index.html", sizeof(full_path));
    } else {
        strlcat(full_path, req->uri, sizeof(full_path));
    }

    const char* ctype = get_content_type(full_path);
    return serve_file(req, full_path, ctype);
}

/// @brief Returns a redirect response to the specified URL, needed for the captive portal
/// @param req Pointer to the HTTP request structure containing request details
/// @return http 301 redirect response
static esp_err_t connectivity_check(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Location", "http://lightbar2api/index.html");
    httpd_resp_set_status(req, "301 Moved Permanently");
    esp_err_t status = httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
    return status;
}

/// @brief Default handler for undefined URIs
/// @param req Pointer to the HTTP request structure containing request details
/// @param err The HTTP error code associated with the request
/// @return HTTP response code indicating success or failure of request processing
static esp_err_t default_handler(httpd_req_t* req, httpd_err_code_t err) {
    ESP_LOGW(TAG, "Undefined URI requested: %s | Error code: %d", req->uri, err);
    const char resp[] = "404 Not Found";
    esp_err_t status = httpd_resp_set_status(req, "404 Not Found");
    if (status == ESP_OK) {
        status = httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }
    return status;
}

/// @brief Start the webserver
/// @param void
/// @return httpd_handle_t The handle to the started webserver instance, or NULL on failure
httpd_handle_t webserver_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Optimized configuration
    config.stack_size = 6144;
    config.max_open_sockets = 5;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 2;
    config.send_wait_timeout = 2;
    config.keep_alive_enable = false;
    config.max_uri_handlers = 50;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_resp_headers = 5;

    ESP_LOGI(TAG, "Webserver starting on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        static httpd_uri_t connectivity_uris[] = {
            {.uri = "/generate_204", .method = HTTP_GET, .handler = connectivity_check, .user_ctx = NULL},
            {.uri = "/library/test/success.html", .method = HTTP_GET, .handler = connectivity_check, .user_ctx = NULL},
            {.uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = connectivity_check, .user_ctx = NULL},
        };

        for (size_t i = 0; i < sizeof(connectivity_uris) / sizeof(connectivity_uris[0]); i++) {
            httpd_register_uri_handler(server, &connectivity_uris[i]);
        }

        char api_key[129] = {0};
        if (config_load_api_key(api_key, sizeof(api_key))) {
            api_init_auth(api_key);
        } else {
            api_init_auth(NULL);
        }

        register_api_v1_endpoints(server);

        httpd_uri_t static_handler = {
            .uri = "/*", .method = HTTP_GET, .handler = static_file_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &static_handler);

        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, default_handler);

        return server;
    }

    ESP_LOGE(TAG, "Error on webserver boot");
    return NULL;
}
