#include "mem/pvec.h"
#include "mem/mem.h"

#include <stdlib.h>

pvec_t *pvec_init() {
    pvec_t *v = malloc(sizeof *v);
    *v = (pvec_t) {
        .items = nullptr,
        .size = 0,
        .capacity = 0
    };
    return v;
}

void pvec_push(pvec_t *v, void *p) {
    if(v->capacity == 0 || v->size >= v->capacity) {
        size_t new_capacity = CAPACITY_GROW(v->capacity);
        void **new_items = malloc(new_capacity * sizeof *new_items);
        for(size_t i = 0; i < v->size; i++) {
            new_items[i] = v->items[i];
        }
        free(v->items);

        v->items = new_items;
        v->capacity = new_capacity;
    }

    v->items[v->size] = p;
    v->size++;
}

void pvec_destroy(pvec_t *v) {
    free(v->items);
    free(v);
}

void *pvec_unwrap(pvec_t *vec, arena_t *a) {
    void **items = arena_copy(a, vec->items, vec->size * sizeof *items);
    pvec_destroy(vec);
    return items;
}
