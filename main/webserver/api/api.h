#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_chip_info.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_netif.h>

void register_api_v1_endpoints(httpd_handle_t server);
