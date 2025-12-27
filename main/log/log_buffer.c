#include "log_buffer.h"

static char* log_buffer = NULL;
static size_t buffer_size = 0;
static size_t write_pos = 0;
static size_t read_pos = 0;
static bool buffer_full = false;
static SemaphoreHandle_t log_mutex = NULL;

/// @brief Initializes a log buffer with the specified size
/// @param size The size of the log buffer to allocate in bytes
/// @return Pointer to the newly created log buffer, or NULL if allocation fails
void log_buffer_init(size_t size) {
    if (log_buffer != NULL) {
        return;
    }

    buffer_size = size;
    log_buffer = (char*)malloc(buffer_size);
    if (log_buffer == NULL) {
        return;
    }

    memset(log_buffer, 0, buffer_size);
    write_pos = 0;
    read_pos = 0;
    buffer_full = false;

    log_mutex = xSemaphoreCreateMutex();
}

/// @brief Logs a message to the buffer
/// @param message The message string to be logged
/// @return Status code indicating success or failure of the logging operation
void log_buffer_add(const char* message) {
    if (log_buffer == NULL || message == NULL) {
        return;
    }

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    size_t msg_len = strlen(message);

    if (msg_len >= buffer_size) {
        msg_len = buffer_size - 1;
    }

    for (size_t i = 0; i < msg_len; i++) {
        log_buffer[write_pos] = message[i];
        write_pos = (write_pos + 1) % buffer_size;

        if (write_pos == read_pos && buffer_full) {
            read_pos = (read_pos + 1) % buffer_size;
        }
    }

    if (write_pos <= read_pos && msg_len > 0) {
        buffer_full = true;
    }

    xSemaphoreGive(log_mutex);
}

/// @brief Initialize a log buffer with a maximum size
/// @param max_size The maximum number of bytes the log buffer can hold
/// @return Pointer to the newly created log buffer, or NULL if allocation fails
size_t log_buffer_get(char* output, size_t max_size) {
    if (log_buffer == NULL || output == NULL || max_size == 0) {
        return 0;
    }

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }

    size_t bytes_written = 0;
    size_t current_pos = read_pos;

    if (buffer_full) {
        while (current_pos != write_pos && bytes_written < max_size - 1) {
            output[bytes_written++] = log_buffer[current_pos];
            current_pos = (current_pos + 1) % buffer_size;
        }
    } else {
        current_pos = 0;
        while (current_pos < write_pos && bytes_written < max_size - 1) {
            output[bytes_written++] = log_buffer[current_pos];
            current_pos++;
        }
    }

    output[bytes_written] = '\0';
    xSemaphoreGive(log_mutex);

    return bytes_written;
}

/// @brief Initializes and manages a circular buffer for logging operations
/// @return void
void log_buffer_clear(void) {
    if (log_buffer == NULL) {
        return;
    }

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    memset(log_buffer, 0, buffer_size);
    write_pos = 0;
    read_pos = 0;
    buffer_full = false;

    xSemaphoreGive(log_mutex);
}
