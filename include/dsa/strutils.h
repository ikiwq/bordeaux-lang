#ifndef STRUTILS_H
#define STRUTILS_H

#include <stddef.h>

typedef enum {
    STR_INVALID_BOOL,
    STR_TRUE,
    STR_FALSE
} to_bool_result_t;

bool is_whitespace(char c);
bool is_alpha(char c);
bool is_numeric(char c);
to_bool_result_t to_bool(const char *s);
size_t str_hash(const char *s);
bool str_eq(const char *s1, const char *s2);

#endif // STRUTILS_H
