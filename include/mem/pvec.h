#ifndef BORDEAUX_PTR_H
#define BORDEAUX_PTR_H

#include <stddef.h>

#include "mem/arena.h"

typedef struct {
    void **items;
    size_t size, capacity;
} pvec_t;

pvec_t *pvec_init();
void pvec_push(pvec_t *v, void *p);
void pvec_destroy(pvec_t *v);
void *pvec_unwrap(pvec_t *vec, arena_t *a);

#endif
