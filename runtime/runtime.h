#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *ptr;
    size_t len;
} str_t;

str_t str_concat(str_t s1, str_t s2) {
    size_t len = s1.len + s2.len;
    char *mem = malloc(len + 1);
    if (!mem) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    memcpy(mem, s1.ptr, s1.len);
    memcpy(mem + s1.len, s2.ptr, s2.len);
    mem[len] = '\0';

    return (str_t){.ptr = mem, .len = len};
}

void print(str_t s) { fwrite(s.ptr, 1, s.len, stdout); }

void println(str_t s) {
    fwrite(s.ptr, 1, s.len, stdout);
    fputc('\n', stdout);
}

str_t itoa(int i) {
    char buf[32];
    int len = snprintf(buf, sizeof buf, "%d", i);

    char *mem = malloc((size_t)len + 1);
    memcpy(mem, buf, (size_t)len + 1);

    return (str_t){.ptr = mem, .len = (size_t)len};
}
