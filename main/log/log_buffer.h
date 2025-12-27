#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void log_buffer_init(size_t buffer_size);
void log_buffer_add(const char* message);
size_t log_buffer_get(char* output, size_t max_size);
void log_buffer_clear(void);
