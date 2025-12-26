#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <esp_vfs_fat.h>

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <esp_flash.h>
#include "storage.h"

const char* STORAGE_TAG = "STORAGE";

/// @brief Initialize the storage
/// @param void
/// @return bool : true if the storage is initialized, false otherwise
bool storage_init(void) {
    ESP_LOGI("STORAGE", "Initializing storage...");

    nvs_init();
    check_nvs_space();
    spiffs_init();
    spiffs_check_space();

    return true;
}

/// @brief Create directories
/// @param path The path to create directories
/// @return bool : true if the directories are created, false otherwise
bool storage_create_directories(const char* path) {
    char temp[256];
    char* p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '/') {
        temp[len - 1] = 0;
    }

    if (storage_has_extension(temp)) {
        char* last_slash = strrchr(temp, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        }
    }

    for (p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(temp, S_IRWXU) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }

    if (mkdir(temp, S_IRWXU) != 0 && errno != EEXIST) {
        return false;
    }

    return true;
}

/// @brief Return if a path has an extension
/// @param path The path to check
/// @return bool : true if the path has an extension, false otherwise
bool storage_has_extension(const char* path) {
    const char* dot = strrchr(path, '.');
    if (!dot || dot == path) return false;
    return true;
}

/// @brief Show tree of files and directories
/// @param base_path The base path (ex. SD_MOUNT_POINT).
/// @param depth The depth level (to use recursively, start at 0).
/// @return void
void storage_list_tree(const char* base_path, int depth) {
    DIR* dir = opendir(base_path);
    if (dir == NULL) {
        ESP_LOGE(STORAGE_TAG, "Impossible to open dir %s", base_path);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        for (int i = 0; i < depth; i++) {
            printf("    ");
        }
        printf("├── %s\n", entry->d_name);

        char full_path[256];
        full_path[0] = '\0';
        strlcpy(full_path, base_path, sizeof(full_path));
        strlcat(full_path, "/", sizeof(full_path));
        strlcat(full_path, entry->d_name, sizeof(full_path));

        if (entry->d_type == DT_DIR) {
            storage_list_tree(full_path, depth + 1);
        }
    }
    closedir(dir);
}
