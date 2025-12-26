#include "captive_portal.h"

#define DNS_PORT 53
#define DNS_MAX_PACKET_SIZE 512

static const char *TAG = "CAPTIVE_PORTAL";
static TaskHandle_t dns_task_handle = NULL;
static int dns_socket = -1;

static int captive_portal_build_dns_response(const uint8_t *request, int request_len, uint8_t *response,
                                             const char *redirect_ip) {
    if (request_len < 12) return 0;

    memcpy(response, request, request_len);

    response[2] = 0x81;
    response[3] = 0x80;
    response[6] = 0x00;
    response[7] = 0x01;

    int pos = request_len;
    response[pos++] = 0xC0;
    response[pos++] = 0x0C;
    response[pos++] = 0x00;
    response[pos++] = 0x01;
    response[pos++] = 0x00;
    response[pos++] = 0x01;
    response[pos++] = 0x00;
    response[pos++] = 0x00;
    response[pos++] = 0x00;
    response[pos++] = 0x3C;
    response[pos++] = 0x00;
    response[pos++] = 0x04;

    uint8_t ip[4];
    sscanf(redirect_ip, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);

    response[pos++] = ip[0];
    response[pos++] = ip[1];
    response[pos++] = ip[2];
    response[pos++] = ip[3];

    return pos;
}

static void captive_portal_dns_server_task(void *pvParameters) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    uint8_t buf[DNS_MAX_PACKET_SIZE];
    uint8_t response[DNS_MAX_PACKET_SIZE];

    dns_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (dns_socket < 0) {
        ESP_LOGE(TAG, "Socket creation error: %d", errno);
        dns_task_handle = NULL;
        vTaskDelete(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DNS_PORT);

    if (bind(dns_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Bind error: %d", errno);
        close(dns_socket);
        dns_socket = -1;
        dns_task_handle = NULL;
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);

    while (1) {
        int len = recvfrom(dns_socket, buf, DNS_MAX_PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom error: %d", errno);
            break;
        }

        int resp_len = captive_portal_build_dns_response(buf, len, response, "10.0.1.1");
        if (sendto(dns_socket, response, resp_len, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            ESP_LOGE(TAG, "sendto error: %d", errno);
        }
    }

    close(dns_socket);
    dns_socket = -1;
    dns_task_handle = NULL;
    vTaskDelete(NULL);
}

/// @brief start the captive portal task
/// @param  void
/// @return void
void captive_portal_start_dns_server(void) {
    if (dns_task_handle != NULL) {
        ESP_LOGW(TAG, "DNS server already running");
        return;
    }

    xTaskCreate(captive_portal_dns_server_task, "dns_server", 4096, NULL, 5, &dns_task_handle);
}

/// @brief stop the captive portal task
/// @param  void
/// @return void
void captive_portal_stop_dns_server(void) {
    if (dns_task_handle != NULL) {
        if (dns_socket != -1) {
            shutdown(dns_socket, 0);
            close(dns_socket);
            dns_socket = -1;
        }
        vTaskDelete(dns_task_handle);
        dns_task_handle = NULL;
        ESP_LOGI(TAG, "DNS server stopped");
    }
}
