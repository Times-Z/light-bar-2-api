#include "log_hook.h"

static vprintf_like_t original_log_function = NULL;

/// @brief Handles logging output by intercepting and processing formatted messages
/// @param fmt Format string for the log message (printf-style format specifiers)
/// @param args Variable argument list containing values to be formatted according to fmt
/// @return Number of characters written to the log output, or error code if logging fails
static int custom_log_vprintf(const char* fmt, va_list args) {
    char message[256];
    int len = vsnprintf(message, sizeof(message), fmt, args);

    if (len > 0) {
        log_buffer_add(message);
    }

    if (original_log_function != NULL) {
        va_list args_copy;
        va_copy(args_copy, args);
        int result = original_log_function(fmt, args_copy);
        va_end(args_copy);
        return result;
    }

    return len;
}
/// @brief Hook function for logging system initialization and configuration
/// @return Returns 0 on success, non-zero error code on failure
void log_hook_init(void) {
    log_buffer_init(8192);

    original_log_function = esp_log_set_vprintf(custom_log_vprintf);
}
