#include <esp_spiffs.h>
#include <esp_log.h>

#define CONFIG_PARTITION_LABEL "config"
#define WWW_PARTITION_LABEL "www"
#define CONFIG_MOUNTPOINT "/config"
#define WWW_MOUNTPOINT "/www"

void spiffs_init(void);
void spiffs_deinit(void);
void spiffs_check_space(void);
