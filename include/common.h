#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include "dsa/vector.h"

VEC_DEFINE_H(size_t, line)

typedef struct {
    const char *path;
    const char *src;
    size_t len;
    line_vec_t line_vec;
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

void print_fe_err(src_file_t, fe_err_t);

#endif // COMMON_H
