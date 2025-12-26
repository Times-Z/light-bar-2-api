#pragma once

typedef enum { JSON_TYPE_STRING, JSON_TYPE_NUMBER, JSON_TYPE_BOOL, JSON_TYPE_RAW } json_type_t;

typedef struct {
    const char *key;
    json_type_t type;
    const void *value;
} json_entry_t;

#define JSON_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

char *build_json_safe(int count, const json_entry_t *entries);
