#include <string.h>
#include <errno.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void captive_portal_start_dns_server(void);
void captive_portal_stop_dns_server(void);
