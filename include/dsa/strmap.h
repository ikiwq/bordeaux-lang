#ifndef STRMAP_H
#define STRMAP_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hashmap.h"
#include "strutils.h"

#ifndef MAP_INITIAL_CAPACITY
#define MAP_INITIAL_CAPACITY 32
#endif // MAP_INITIAL_CAPACITY

#ifndef MAP_RESIZE_RATIO
#define MAP_RESIZE_RATIO 0.75
#endif // MAP_RESIZE_RATIO

#ifndef MAP_RESIZE_FACTOR
#define MAP_RESIZE_FACTOR 2
#endif // MAP_RESIZE_FACTOR

#define MAP_CAT_(a, b)          a##b
#define MAP_CAT(a, b)           MAP_CAT_(a, b)

#define MAP_ENTRY(name)         MAP_CAT(name, _entry_t)
#define MAP_STRUCT(name)        MAP_CAT(name, _map_t)
#define MAP_FN(name, suffix)    MAP_CAT(MAP_CAT(name, _map), suffix)

#define MAP_DIE(msg, code) \
    do { \
        fprintf(stderr, "strmap: " msg "\n"); \
        exit(code); \
    } while(0)

#define STRMAP_DEFINE_H(T, name) \
    typedef struct { \
        entry_status_t status; \
        char *key; \
        T value; \
    } MAP_ENTRY(name);\
    \
    typedef struct { \
        MAP_ENTRY(name) *entries; \
        size_t size; \
        size_t capacity; \
    } MAP_STRUCT(name); \
    \
    static inline MAP_STRUCT(name) MAP_FN(name, _init)(void) { \
        MAP_ENTRY(name) *entries = calloc(MAP_INITIAL_CAPACITY, sizeof(MAP_ENTRY(name))); \
        if(entries== nullptr) \
            MAP_DIE("cannot initialize strmap: insufficient memory", ENOMEM); \
    \
        return (MAP_STRUCT(name)) { \
            .entries = entries, \
            .size = 0, \
            .capacity = MAP_INITIAL_CAPACITY \
        }; \
    } \
    \
    static inline void MAP_FN(name, _resize)(MAP_STRUCT(name) *map, size_t new_capacity) { \
        if(!map) \
            MAP_DIE("received invalid map while resizing", EINVAL); \
        if (new_capacity < map->size) \
            MAP_DIE("refusing to resize below current size", EINVAL); \
        if (new_capacity > SIZE_MAX / sizeof(T)) \
            MAP_DIE("cannot resize vector: capacity overflow", ENOMEM); \
    \
        if(new_capacity == 0) { \
            free(map->entries); \
            map->entries = nullptr; \
            map->capacity = 0; \
            map->size = 0; \
            return; \
        } \
    \
        MAP_ENTRY(name) *new_entries = calloc(new_capacity, sizeof(MAP_ENTRY(name))); \
        if(!new_entries) \
            MAP_DIE("cannot resize map: insufficient memory", ENOMEM); \
    \
        size_t live = 0; \
        for (size_t i = 0; i < map->capacity; i++) { \
            MAP_ENTRY(name) *old = &map->entries[i]; \
            if (old->status != ENTRY_INSERTED) continue; \
    \
            const size_t hash = str_hash(old->key); \
            bool placed = false; \
            for(size_t it = 0; it < new_capacity; it++) { \
                const size_t index = (hash + it) % new_capacity; \
                if (new_entries[index].status == ENTRY_INSERTED) continue; \
                \
                new_entries[index].status = ENTRY_INSERTED; \
                new_entries[index].key = old->key; \
                new_entries[index].value = old->value; \
                placed = true; \
                break; \
            } \
            if (!placed) \
                MAP_DIE("cannot resize map: no free slot during rehash", EXIT_FAILURE); \
            live++; \
        } \
    \
        free(map->entries); \
        map->entries = new_entries; \
        map->capacity = new_capacity; \
        map->size = live; \
    } \
    \
static inline void MAP_FN(name, _put)(MAP_STRUCT(name) *map, const char *key, T value) { \
        if (!map || map->capacity == 0) \
            MAP_DIE("received invalid map while putting entry", EXIT_FAILURE); \
        if (!key) \
            MAP_DIE("refusing to put entry with null key", EXIT_FAILURE); \
    \
        if ((double) map->size / (double) map->capacity > MAP_RESIZE_RATIO) \
            MAP_FN(name, _resize)(map, MAP_RESIZE_FACTOR * map->capacity); \
    \
        const size_t hash = str_hash(key); \
        size_t tombstone = map->capacity; \
        for (size_t it = 0; it < map->capacity; it++) { \
            const size_t index = (hash + it) % map->capacity; \
            MAP_ENTRY(name) *e = &map->entries[index]; \
    \
            if (e->status == ENTRY_INSERTED) { \
                if (strcmp(e->key, key) == 0) { e->value = value; return; } \
                continue; \
            } \
            if (e->status == ENTRY_DELETED) { \
                if (tombstone == map->capacity) tombstone = index; \
                continue; \
            } \
            const size_t target = (tombstone != map->capacity) ? tombstone : index; \
            char *dup = strdup(key); \
            if (!dup) MAP_DIE("cannot put inside map: insufficient memory", EXIT_FAILURE); \
    \
            map->entries[target].status = ENTRY_INSERTED; \
            map->entries[target].key    = dup; \
            map->entries[target].value  = value; \
            map->size++; \
            return; \
        } \
        if (tombstone != map->capacity) { \
            char *dup = strdup(key); \
            if (!dup) MAP_DIE("cannot put inside map: insufficient memory", EXIT_FAILURE); \
            map->entries[tombstone].status = ENTRY_INSERTED; \
            map->entries[tombstone].key    = dup; \
            map->entries[tombstone].value  = value; \
            map->size++; \
            return; \
        } \
        MAP_DIE("cannot put inside map: no free slot found", EXIT_FAILURE); \
    } \
    \
    static inline T *MAP_FN(name, _get)(const MAP_STRUCT(name) *map, const char *key) { \
        if (!map || map->capacity == 0) \
            MAP_DIE("received invalid map while getting entry", EXIT_FAILURE); \
        if (!key) \
            MAP_DIE("refusing to get entry with null key", EXIT_FAILURE); \
    \
        const size_t hash = str_hash(key); \
        for (size_t it = 0; it < map->capacity; it++) { \
            const size_t index = (hash + it) % map->capacity; \
            MAP_ENTRY(name) *e = &map->entries[index]; \
    \
            if (e->status == ENTRY_EMPTY) return nullptr; \
            if (e->status == ENTRY_DELETED) continue; \
            if (strcmp(e->key, key) == 0) return &e->value; \
        } \
        return nullptr; \
    } \
    \
    static inline T *MAP_FN(name, _destroy)(MAP_STRUCT(name) *map) { \
        free(map->entries); \
        map->size = 0; \
        map->capacity = 0; \
    }

#endif // STRMAP_H
