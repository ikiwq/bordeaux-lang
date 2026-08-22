// A vector implementation that uses arena instead of heap.
// Since reallocating in an arena is costly (due to the difficulty
// of freeing data before the current cursor) each vector has a linked list
// of "chunks", which hold data in a semi contiguous style


// Fixes for IDE. Ideally these should return an error while compiling
#ifndef AVEC_TYPE
#define AVEC_TYPE int
#endif
#ifndef AVEC_NAME
#define AVEC_NAME int
#endif


#ifndef AVEC_CHUNK_CAPACITY
#define AVEC_CHUNK_CAPACITY 8
#endif // AVEC_CHUNK_CAPACITY


#ifndef AVEC_COMMON_H
#define AVEC_COMMON_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dsa/common.h"
#include "mem/arena.h"

#define AVEC_DIE(msg, err) \
    do { \
        fputs((msg), stderr); \
        exit((err)); \
    } while (0)

#define AVEC_FOREACH(item, avec_ptr) \
    for (size_t CONCAT(_avec_i, __LINE__) = 0, \
                CONCAT(_avec_n, __LINE__) = (avec_ptr)->size; \
         CONCAT(_avec_i, __LINE__) < CONCAT(_avec_n, __LINE__); \
         CONCAT(_avec_i, __LINE__)++) \
        for (typeof((avec_ptr)->data[0]) item = (avec_ptr)->data[CONCAT(_avec_i, __LINE__)], \
                 *CONCAT(_avec_once, __LINE__) = (void*)1; \
             CONCAT(_avec_once, __LINE__); \
             CONCAT(_avec_once, __LINE__) = (void*)0)

#endif // AVEC_COMMON_H


#define AVEC                CONCAT(AVEC_NAME, _avec)
#define AVEC_T              CONCAT(AVEC, _t)
#define AVEC_CHUNK          CONCAT(AVEC_NAME, _chunk)
#define AVEC_CHUNK_T        CONCAT(AVEC_CHUNK, _t)
#define AVEC_FUN(name)      CONCAT(CONCAT(AVEC, _), name)

typedef struct {
    arena_t *arena;
    AVEC_TYPE *data;
    size_t size;
    size_t capacity;
} AVEC_T;

static AVEC_T *AVEC_FUN(make)(arena_t *arena) {
    AVEC_T *avec = arena_alloc(arena, sizeof *avec);
    *avec = (AVEC_T) {
        .arena = arena,
        .data = nullptr,
        .capacity = 0,
        .size = 0
    };
    return avec;
}

static AVEC_T *AVEC_FUN(make_cap)(arena_t *arena, size_t initial_capacity) {
    AVEC_T *avec = arena_alloc(arena, sizeof *avec);
    AVEC_TYPE *data = initial_capacity > 0
        ? arena_alloc(arena, initial_capacity * sizeof *data)
        : nullptr;
    *avec = (AVEC_T) {
        .arena = arena,
        .data = data,
        .capacity = initial_capacity,
        .size = 0
    };
    return avec;
}

static void AVEC_FUN(resize)(AVEC_T *avec) {
    size_t new_cap = avec->capacity == 0 ? 8 : avec->capacity * 2;
    size_t old_bytes = avec->capacity * sizeof(AVEC_TYPE);
    size_t new_bytes = new_cap * sizeof(AVEC_TYPE);

    char *end_of_this_alloc = (char*)avec->data + old_bytes;
    if (avec->data != nullptr && end_of_this_alloc == avec->arena->base_ptr + avec->arena->used) {
        arena_alloc(avec->arena, new_bytes - old_bytes);
        avec->capacity = new_cap;
        return;
    }

    AVEC_TYPE *new_data = arena_alloc(avec->arena, new_bytes);
    if (avec->data != nullptr) 
        memcpy(new_data, avec->data, avec->size * sizeof *new_data);
    avec->data = new_data;
    avec->capacity = new_cap;
}

static void AVEC_FUN(push)(AVEC_T *avec, AVEC_TYPE item) {
    if (!avec) AVEC_DIE("cannot push item to null arena_vector", EINVAL);

    if (avec->size >= avec->capacity) {
        AVEC_FUN(resize)(avec);
    }
    
    avec->data[avec->size++] = item;
}

static AVEC_TYPE AVEC_FUN(pop)(AVEC_T *avec) {
    if (!avec) AVEC_DIE("cannot pop item from null arena_vector", EINVAL);
    if (avec->size == 0) AVEC_DIE("cannot pop from empty arena_vector", EINVAL);

    avec->size--;
    return avec->data[avec->size];
}

static AVEC_TYPE AVEC_FUN(at)(AVEC_T *avec, size_t i) {
    if(!avec) AVEC_DIE("cannot get item from null arena_vector", EINVAL);
    if(i >= avec->size) AVEC_DIE("given index exceeds arena_vector size", EINVAL);
    
    return avec->data[i];
}

static AVEC_TYPE AVEC_FUN(last)(AVEC_T *avec) {
    if(!avec) AVEC_DIE("cannot get last item from null arena_vector", EINVAL);
    if(avec->size == 0) AVEC_DIE("cannot get last item from empty arena_vector", EINVAL);

    return avec->data[avec->size - 1];
}

#undef AVEC_FUN
#undef AVEC_CHUNK_T
#undef AVEC_CHUNK
#undef AVEC_T
#undef AVEC
#undef AVEC_NAME
#undef AVEC_TYPE
#undef AVEC_CHUNK_CAPACITY

