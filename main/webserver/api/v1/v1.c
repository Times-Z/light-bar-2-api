#include "v1.h"
#include "helper/auth.h"

esp_err_t status_handler(httpd_req_t* req) {
    uint32_t free_heap = esp_get_free_heap_size();

    int64_t uptime_us = esp_timer_get_time();
    int uptime_sec = uptime_us / 1000000;

    char free_heap_str[64];
    if (free_heap < 1024 * 1024) {
        snprintf(free_heap_str, sizeof(free_heap_str), "%.2f KB", free_heap / 1024.0);
    } else {
        snprintf(free_heap_str, sizeof(free_heap_str), "%.2f MB", free_heap / (1024.0 * 1024.0));
    }

    int days = uptime_sec / 86400;
    int hours = (uptime_sec % 86400) / 3600;
    int minutes = (uptime_sec % 3600) / 60;
    int seconds = uptime_sec % 60;
    char uptime_str[64];
    snprintf(uptime_str, sizeof(uptime_str), "%dd %02dh %02dm %02ds", days, hours, minutes, seconds);

    time_t now;
    time(&now);

    json_entry_t entries[] = {{"status", JSON_TYPE_STRING, "ok"},
                              {"sys_timestamp", JSON_TYPE_NUMBER, &now},
                              {"ip", JSON_TYPE_STRING, wifi_get_current_ip_str()},
                              {"main_dns", JSON_TYPE_STRING, wifi_get_current_dns_str()},
                              {"free_heap", JSON_TYPE_STRING, free_heap_str},
                              {"uptime", JSON_TYPE_STRING, uptime_str}};

    char* json_response = build_json_safe(JSON_ARRAY_SIZE(entries), entries);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t status = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);
    return status;
}

esp_err_t wifi_scan_handler(httpd_req_t* req) {
    wifi_ap_record_t* ap_records = NULL;
    uint16_t ap_count = 0;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    esp_err_t err = wifi_scan_networks(&ap_records, &ap_count);
    if (err != ESP_OK || ap_records == NULL || ap_count == 0) {
        const char* msg = "Wi-Fi scan failed";
        const char* err_name = esp_err_to_name(err);

        json_entry_t error_json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, msg},
            {"error", JSON_TYPE_STRING, err_name},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(error_json), error_json);

        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    char* networks_json = malloc(2048);

    networks_json[0] = '[';
    networks_json[1] = '\0';

    for (int i = 0; i < ap_count; i++) {
        char ap_entry[256];
        char bssid_str[18];
        const char* auth_str = authmode_to_str(ap_records[i].authmode);

        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", ap_records[i].bssid[0],
                 ap_records[i].bssid[1], ap_records[i].bssid[2], ap_records[i].bssid[3], ap_records[i].bssid[4],
                 ap_records[i].bssid[5]);

        snprintf(ap_entry, sizeof(ap_entry),
                 "{\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d,\"authmode\":\"%s\",\"bssid\":\"%s\"}",
                 ap_records[i].ssid, ap_records[i].rssi, ap_records[i].primary, auth_str, bssid_str);

        strcat(networks_json, ap_entry);
        if (i < ap_count - 1) strcat(networks_json, ",");
    }

    strcat(networks_json, "]");

    json_entry_t final_json[] = {
        {"success", JSON_TYPE_BOOL, &(int){1}},
        {"networks", JSON_TYPE_RAW, networks_json},
    };
    char* json_response = build_json_safe(JSON_ARRAY_SIZE(final_json), final_json);

    free(networks_json);
    ap_records = NULL;

    esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);
    return res;
}

esp_err_t wifi_connect_handler(httpd_req_t* req) {
    should_save_credentials = true;
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (ret <= 0) {
        json_entry_t json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, "No body received"},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(json), json);

        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    buf[ret] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};

    char* ssid_ptr = strstr(buf, "\"ssid\"");
    char* pass_ptr = strstr(buf, "\"password\"");

    if (ssid_ptr) sscanf(ssid_ptr, "\"ssid\":\"%32[^\"]", ssid);
    if (pass_ptr) sscanf(pass_ptr, "\"password\":\"%64[^\"]", password);

    if (strlen(ssid) == 0) {
        json_entry_t error_json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, "Missing or invalid SSID"},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(error_json), error_json);
        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    if (!wifi_start_sta(ssid, password)) {
        json_entry_t json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, "Failed to connect..."},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(json), json);

        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    json_entry_t success_json[] = {{"success", JSON_TYPE_BOOL, &(int){1}},
                                   {
                                       "message",
                                       JSON_TYPE_STRING,
                                       "Wi-Fi connection initiated",
                                   }};

    char* json_response = build_json_safe(JSON_ARRAY_SIZE(success_json), success_json);
    esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);
    return res;
}

esp_err_t ntp_set_handler(httpd_req_t* req) {
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (ret <= 0) {
        json_entry_t json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, "No body received"},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(json), json);

        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    buf[ret] = '\0';
    char ntp_domain[33] = {0};

    char* ntp_domain_ptr = strstr(buf, "\"ntp_domain\"");

    if (ntp_domain_ptr) {
        ntp_domain_ptr = strchr(ntp_domain_ptr, ':');
        if (ntp_domain_ptr) {
            ntp_domain_ptr = strchr(ntp_domain_ptr, '"');
            if (ntp_domain_ptr) {
                ntp_domain_ptr++;
                sscanf(ntp_domain_ptr, "%32[^\"]", ntp_domain);
            }
        }
    }

    if (strlen(ntp_domain) == 0) {
        json_entry_t error_json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, ntp_domain},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(error_json), error_json);
        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    if (!time_sync_with_ntp(ntp_domain)) {
        json_entry_t json[] = {
            {"success", JSON_TYPE_BOOL, &(int){0}},
            {"message", JSON_TYPE_STRING, "Failed to sync time with NTP"},
        };

        char* json_response = build_json_safe(JSON_ARRAY_SIZE(json), json);

        esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
        return res;
    }

    json_entry_t success_json[] = {{"success", JSON_TYPE_BOOL, &(int){1}},
                                   {
                                       "message",
                                       JSON_TYPE_STRING,
                                       "NTPS sync ok",
                                   }};

    char* json_response = build_json_safe(JSON_ARRAY_SIZE(success_json), success_json);
    esp_err_t res = httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);
    return res;
}
