#ifndef BORDEAUX_COMMON_H
#define BORDEAUX_COMMON_H

#include <stddef.h>

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
    const char *path;
    const char *src;
    size_t len;
} src_file_t;

typedef struct {
    size_t start;
    size_t end;
} span_t;

void errors_init(src_file_t src_file);
void report_err(span_t span, const char *fmt, ...);

#endif // BORDEAUX_COMMON_H
