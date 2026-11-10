#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

extern void bdx_main();

typedef struct {
    const char *ptr;
    size_t len;
} string;

void print(string str) {
    printf("%.*s", (int)str.len, str.ptr);
}

string intstr(int i) {
    size_t len = snprintf(nullptr, 0, "%d", i);
    char *ptr = malloc(len + 1);
    snprintf(ptr, len + 1, "%d", i);

    return (string) {
        .len = len,
        .ptr = ptr
    };
}

int main() {
    bdx_main();
    return 0;
}
