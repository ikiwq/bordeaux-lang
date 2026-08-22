// A hashmap implementation that lives on the heap,
// where the key is always an array of characters (a string)

// Fixes for IDE. Ideally these should return an error while compiling
#ifndef STRMAP_TYPE
#define STRMAP_TYPE int
#endif
#ifndef STRMAP_NAME
#define STRMAP_NAME int
#endif

#ifndef STRMAP_DEFAULT_CAPACITY
#define STRMAP_DEFAULT_CAPACITY 64
#endif

#ifndef STRMAP_RESIZE_RATIO
#define STRMAP_RESIZE_RATIO 0.7
#endif

#ifndef STRMAP_RESIZE_FACTOR
#define STRMAP_RESIZE_FACTOR 2
#endif

#ifndef STRMAP_COMMON_H
#define STRMAP_COMMON_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "dsa/common.h"
#include "dsa/strutils.h"

typedef enum {
    ENTRY_EMPTY,
    ENTRY_INSERTED,
    ENTRY_DELETED
} entry_status_t;

#define MAP_DIE(msg, code) \
    do { \
        fprintf(stderr, "strmap: " msg "\n"); \
    } while(0)

#endif // STRMAP_COMMON_H

#define MAP_ENTRY   CONCAT(STRMAP_NAME, _entry)
#define MAP_ENTRY_T CONCAT(MAP_ENTRY, _t)
#define MAP         CONCAT(STRMAP_NAME, _map)
#define MAP_T       CONCAT(MAP, _t)
#define MAP_FN(n)   CONCAT(MAP, CONCAT(_, n))

typedef struct {
    entry_status_t status;
    char *key;
    STRMAP_TYPE value;
} MAP_ENTRY_T;

typedef struct {
    MAP_ENTRY_T *entries;
    size_t size;
    size_t capacity;
} MAP_T;

static inline MAP_T* MAP_FN(make)(void) {
    MAP_ENTRY_T *entries = calloc(STRMAP_DEFAULT_CAPACITY, sizeof(MAP_ENTRY_T));
    if (!entries)
        MAP_DIE("cannot initialize strmap: insufficient memory", ENOMEM);

    MAP_T *map = malloc(sizeof(MAP_T));
    if (!map) {
        free(entries);
        MAP_DIE("cannot initialize strmap: insufficient memory", ENOMEM);
    }

    *map = (MAP_T){
        .entries  = entries,
        .capacity = STRMAP_DEFAULT_CAPACITY,
        .size     = 0
    };
    return map;
}

static inline void MAP_FN(resize)(MAP_T *map, size_t new_capacity) {
    if (!map)
        MAP_DIE("received invalid map while resizing", EINVAL);
    if (new_capacity < map->size)
        MAP_DIE("refusing to resize below current size", EINVAL);

    if (new_capacity == 0) {
        for (size_t i = 0; i < map->capacity; i++)
            free(map->entries[i].key);
        free(map->entries);
        map->entries  = NULL;
        map->capacity = 0;
        map->size     = 0;
        return;
    }

    MAP_ENTRY_T *new_entries = calloc(new_capacity, sizeof(MAP_ENTRY_T));
    if (!new_entries)
        MAP_DIE("cannot resize map: insufficient memory", ENOMEM);

    size_t live = 0;
    for (size_t i = 0; i < map->capacity; i++) {
        MAP_ENTRY_T *old = &map->entries[i];
        if (old->status != ENTRY_INSERTED)
            continue;

        const size_t hash = str_hash(old->key);
        for (size_t it = 0; it < new_capacity; it++) {
            const size_t index = (hash + it) % new_capacity;
            if (new_entries[index].status == ENTRY_INSERTED)
                continue;

            new_entries[index] = (MAP_ENTRY_T){
                .status = ENTRY_INSERTED,
                .key    = old->key,
                .value  = old->value
            };
            live++;
            break;
        }
    }

    free(map->entries);
    map->entries  = new_entries;
    map->capacity = new_capacity;
    map->size     = live;
}

static inline void MAP_FN(put)(MAP_T *map, const char *key, STRMAP_TYPE value) {
    if (!map || map->capacity == 0)
        MAP_DIE("received invalid map while putting entry", EXIT_FAILURE);
    if (!key)
        MAP_DIE("refusing to put entry with null key", EXIT_FAILURE);

    if ((double)map->size / (double)map->capacity > STRMAP_RESIZE_RATIO)
        MAP_FN(resize)(map, STRMAP_RESIZE_FACTOR * map->capacity);

    const size_t hash = str_hash(key);
    size_t tombstone = map->capacity;

    for (size_t it = 0; it < map->capacity; it++) {
        const size_t index = (hash + it) % map->capacity;
        MAP_ENTRY_T *e = &map->entries[index];

        if (e->status == ENTRY_INSERTED) {
            if (strcmp(e->key, key) == 0) {
                e->value = value;
                return;
            }
            continue;
        }
        if (e->status == ENTRY_DELETED) {
            if (tombstone == map->capacity)
                tombstone = index;
            continue;
        }

        char *dup = strdup(key);
        if (!dup)
            MAP_DIE("cannot put inside map: insufficient memory", ENOMEM);

        const size_t target = (tombstone != map->capacity) ? tombstone : index;
        map->entries[target] = (MAP_ENTRY_T){
            .status = ENTRY_INSERTED,
            .key    = dup,
            .value  = value
        };
        map->size++;
        return;
    }

    if (tombstone != map->capacity) {
        char *dup = strdup(key);
        if (!dup)
            MAP_DIE("cannot put inside map: insufficient memory", ENOMEM);

        map->entries[tombstone] = (MAP_ENTRY_T){
            .status = ENTRY_INSERTED,
            .key    = dup,
            .value  = value
        };
        map->size++;
        return;
    }

    MAP_DIE("cannot put inside map: no free slot found", EXIT_FAILURE);
}

static inline STRMAP_TYPE* MAP_FN(get)(const MAP_T *map, const char *key) {
    if (!map || map->capacity == 0)
        MAP_DIE("received invalid map while getting entry", EXIT_FAILURE);
    if (!key)
        MAP_DIE("refusing to get entry with null key", EXIT_FAILURE);

    const size_t hash = str_hash(key);
    for (size_t it = 0; it < map->capacity; it++) {
        const size_t index = (hash + it) % map->capacity;
        MAP_ENTRY_T *e = &map->entries[index];

        if (e->status == ENTRY_EMPTY)
            return NULL;
        if (e->status == ENTRY_DELETED)
            continue;
        if (strcmp(e->key, key) == 0)
            return &e->value;
    }
    return NULL;
}

static inline bool MAP_FN(remove)(MAP_T *map, const char *key) {
    if (!map || map->capacity == 0)
        MAP_DIE("received invalid map while removing entry", EXIT_FAILURE);
    if (!key)
        MAP_DIE("refusing to remove entry with null key", EXIT_FAILURE);

    const size_t hash = str_hash(key);
    for (size_t it = 0; it < map->capacity; it++) {
        const size_t index = (hash + it) % map->capacity;
        MAP_ENTRY_T *e = &map->entries[index];

        if (e->status == ENTRY_EMPTY)
            return false;
        if (e->status == ENTRY_DELETED)
            continue;
        if (strcmp(e->key, key) == 0) {
            free(e->key);
            e->key    = NULL;
            e->status = ENTRY_DELETED;
            map->size--;
            return true;
        }
    }
    return false;
}

static inline void MAP_FN(destroy)(MAP_T *map) {
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++)
        free(map->entries[i].key);
    free(map->entries);
    free(map);
}

#undef STRMAP_DEFAULT_CAPACITY
#undef STRMAP_RESIZE_RATIO
#undef STRMAP_RESIZE_FACTOR
#undef MAP_ENTRY
#undef MAP_ENTRY_T
#undef MAP
#undef MAP_T
#undef MAP_FN
#undef MAP_ITER_T
#undef MAP_ITER_FN
#undef STRMAP_TYPE
#undef STRMAP_NAME
