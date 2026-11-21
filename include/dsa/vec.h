#ifndef BORDEAUX_VEC_H
#define BORDEAUX_VEC_H

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem/arena.h"

#define RESIZE_CAPACITY(cap) ((cap) == 0 ? 8 : (cap) * 2)

#endif // BORDEAUX_VEC_H

#ifndef VEC_TYPE
#define VEC_TYPE int
#endif

#ifndef VEC_NAME
#define VEC_NAME int
#endif

#define VEC_STR_(x) #x
#define VEC_STR(x) VEC_STR_(x)

#define VEC_CAT_(a, b) a##b
#define VEC_CAT(a, b) VEC_CAT_(a, b)

#define VEC_PREFIX VEC_CAT(VEC_NAME, _vec_)
#define VEC_STRUCT VEC_CAT(VEC_NAME, _vec_t)
#define VEC_FUN(name) VEC_CAT(VEC_PREFIX, name)

typedef struct {
    VEC_TYPE *data;
    size_t size, capacity;
} VEC_STRUCT;

static inline void VEC_FUN(resize)(VEC_STRUCT *v) {
    if (!v) {
        printf("received nullptr while resizing " VEC_STR(VEC_STRUCT) "\n");
        exit(EINVAL);
    }
    v->capacity = RESIZE_CAPACITY(v->capacity);
    v->data = realloc(v->data, v->capacity * sizeof(VEC_TYPE));
}

static inline void VEC_FUN(push)(VEC_STRUCT *v, VEC_TYPE item) {
    if (v->size == v->capacity) VEC_FUN(resize)(v);
    v->data[v->size] = item;
    v->size++;
}

static inline void VEC_FUN(destroy)(VEC_STRUCT *v) {
    if (!v) {
        printf("received nullptr while destroying " VEC_STR(VEC_STRUCT) "\n");
        exit(EINVAL);
    }
    free(v->data);
    v->data = nullptr;
    v->size = v->capacity = 0;
}

static inline void VEC_FUN(move)(VEC_STRUCT *v, arena_t *a) {
    if (!v || !v->data) {
        printf("received nullptr while moving" VEC_STR(VEC_STRUCT) "\n");
        exit(EINVAL);
    }
    if (v->size == 0) {
        free(v->data);
        v->data = nullptr;
        v->capacity = 0;
    }
    VEC_TYPE *items = arena_copy(a, v->data, v->size * sizeof *items);
    free(v->data);
    v->data = items;
    v->capacity = v->size;
}

#undef VEC_FUN
#undef VEC_STRUCT
#undef VEC_PREFIX
#undef VEC_CAT
#undef VEC_CAT_
#undef VEC_STR
#undef VEC_STR_
#undef VEC_TYPE
#undef VEC_NAME
