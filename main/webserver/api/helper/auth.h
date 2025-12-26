#pragma once

#include <esp_http_server.h>
#include <stdbool.h>

bool auth_validate_api_key(httpd_req_t* req);
esp_err_t auth_send_unauthorized(httpd_req_t* req, const char* message);
void auth_init_from_config(const char* api_key);
