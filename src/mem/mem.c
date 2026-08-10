#include "mem/mem.h"

#include <stdio.h>
#include <stdlib.h>

void *h_malloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    return p;
}
