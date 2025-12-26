#include <stdio.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

extern const char *APP_NAME;

httpd_handle_t webserver_start(void);
