#include "dsa/strutils.h"

#include <string.h>

bool is_whitespace(const char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return true;
        default:
            return false;
    }
}

bool is_alpha(const char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_numeric(const char c) {
    return c >= '0' && c <= '9';
}

to_bool_result_t to_bool(const char *c) {
    const int length = strlen(c);
    switch (c[0]) {
        case 'f': {
            if (length != 5) return STR_INVALID_BOOL;
            if (memcmp(c + 1, "alse", 4) == 0) return STR_FALSE;
            break;
        }
        case 't': {
            if (length != 4) return STR_INVALID_BOOL;
            if (memcmp(c + 1, "rue", 3) == 0) return STR_TRUE;
            break;
        }
        default: break;
    }
    return STR_INVALID_BOOL;
}

size_t str_hash(const char *s) {
    size_t hash = 5381;
    int c;
    while ((c = (unsigned char) *s++)) {
        hash = (hash << 5) + hash + c;
    }

    return hash;
}

bool str_eq(const char *s1, const char *s2) {
    return memcmp(s1, s2, strlen(s1));
}
