#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include "nvs.h"

void time_init();
void time_sync_from_build(void);
bool time_sync_with_ntp(const char *ntp_domain);
