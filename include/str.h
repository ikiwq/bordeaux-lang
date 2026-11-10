#ifndef BORDEAUX_STR_H
#define BORDEAUX_STR_H

#include <stddef.h>
#include <string.h>

typedef struct {
    const char *ptr;
    size_t len;
} str_t;

#define STR_LIT(lit)        ((str_t){ .ptr = (lit), .len = sizeof(lit) - 1})
#define STR(p, n)           ((str_t){ .ptr = (p), .len = (n) })

#define STR_FMT             "%.*s"
#define STR_ARG(s)          (int)(s).len, (s).ptr

static inline bool str_eq(const str_t s1, const str_t s2) {
    return s1.len == s2.len && memcmp(s1.ptr, s2.ptr, s1.len) == 0;
}

static inline size_t str_hash(const str_t s) {
    unsigned long hash = 5381;
    for(size_t i = 0; i < s.len; i++) {
        hash = ((hash << 5) + hash) + s.ptr[i];
    }
    return hash;
}

#endif // BORDEAUX_STR_H
