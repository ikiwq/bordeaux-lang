#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define AVEC_TYPE size_t
#define AVEC_NAME line
#include "dsa/arena_vec.h"

typedef struct {
    const char *path;
    const char *src;
    size_t len;
    line_avec_t *line_avec;
} src_file_t;

typedef struct {
    const char *src;
    size_t length;
    size_t start;
    size_t end;
} span_t;

typedef struct {
    const char *err;
    span_t span;
} fe_err_t;

#define AVEC_TYPE fe_err_t
#define AVEC_NAME err
#include "dsa/arena_vec.h"

void print_fe_errs(src_file_t, const err_avec_t*);

#endif // COMMON_H
