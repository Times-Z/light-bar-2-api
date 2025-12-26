#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_CRITICAL } LogLevel;

void log_message(LogLevel level, const char *message);
extern const char *SD_MOUNT_POINT;
extern const char *APP_NAME;
extern const char *APP_VERSION;

#endif  // LOG_H
