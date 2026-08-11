#ifndef VEC_H
#define VEC

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef VEC_INITIAL_CAPACITY
#define VEC_INITIAL_CAPACITY 8
#endif

#ifndef VEC_RESIZE_FACTOR
#define VEC_RESIZE_FACTOR 2
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) \
    (sizeof(array) / sizeof(*array))
#endif // ARRAY_SIZE

#define VEC_CAT_(a, b) a##b
#define VEC_CAT(a, b) VEC_CAT_(a, b)

#define VEC_STRUCT(name)     VEC_CAT(name, _vec_t)
#define VEC_FN(name, suffix) VEC_CAT(VEC_CAT(name, _vec), suffix)

#define VEC_DIE(msg, code) \
    do { \
        fprintf(stderr, "vector: " msg "\n"); \
        exit(code); \
    } while (0)

#define VEC_DEFINE_H(T, name) \
    typedef struct { \
        T *data; \
        size_t size; \
        size_t capacity; \
    } VEC_STRUCT(name); \
    \
    static inline VEC_STRUCT(name) VEC_FN(name, _init)(void) { \
        T *data = malloc(sizeof(T) * VEC_INITIAL_CAPACITY); \
        if (data == nullptr) \
            VEC_DIE("cannot initialize vector: insufficient memory", ENOMEM); \
    \
        return (VEC_STRUCT(name)) { \
            .data = data, \
            .size = 0, \
            .capacity = VEC_INITIAL_CAPACITY \
        }; \
    } \
    \
    static inline void VEC_FN(name, _resize)(VEC_STRUCT(name) *vec, size_t new_capacity) { \
        if (!vec) \
            VEC_DIE("received invalid vector while resizing", EINVAL); \
        if (new_capacity < vec->size) \
            VEC_DIE("refusing to resize below current size", EINVAL); \
        if (new_capacity > SIZE_MAX / sizeof(T)) \
            VEC_DIE("cannot resize vector: capacity overflow", ENOMEM); \
    \
        if (new_capacity == 0) { \
            free(vec->data); \
            vec->data = nullptr; \
            vec->capacity = 0; \
            return; \
        } \
    \
        T *temp = realloc(vec->data, sizeof(T) * new_capacity); \
        if (temp == nullptr) \
            VEC_DIE("cannot resize vector: insufficient memory", ENOMEM); \
    \
        vec->data = temp; \
        vec->capacity = new_capacity; \
    } \
    \
    static inline void VEC_FN(name, _reserve)(VEC_STRUCT(name) *vec, size_t min_capacity) { \
        if (!vec) \
            VEC_DIE("received invalid vector while reserving", EINVAL); \
        if (vec->capacity < min_capacity) \
            VEC_FN(name, _resize)(vec, min_capacity); \
    } \
    \
    static inline void VEC_FN(name, _push)(VEC_STRUCT(name) *vec, T item) { \
        if (!vec) \
            VEC_DIE("received invalid vector while pushing item", EINVAL); \
    \
        if (vec->size >= vec->capacity) { \
            size_t new_capacity; \
            if (vec->capacity == 0) { \
                new_capacity = VEC_INITIAL_CAPACITY; \
            } else { \
                if (vec->capacity > SIZE_MAX / VEC_RESIZE_FACTOR) \
                    VEC_DIE("cannot grow vector: capacity overflow", ENOMEM); \
                new_capacity = vec->capacity * VEC_RESIZE_FACTOR; \
            } \
            VEC_FN(name, _resize)(vec, new_capacity); \
        } \
    \
        vec->data[vec->size] = item; \
        vec->size++; \
    } \
    \
    static inline T VEC_FN(name, _pop)(VEC_STRUCT(name) *vec) { \
        if (!vec || !vec->data) \
            VEC_DIE("received invalid vector while popping item", EINVAL); \
        if (vec->size == 0) \
            VEC_DIE("cannot pop from an empty vector", EINVAL); \
    \
        vec->size--; \
        return vec->data[vec->size]; \
    } \
    \
    static inline T VEC_FN(name, _peek)(const VEC_STRUCT(name) *vec) { \
        if (!vec || !vec->data) \
            VEC_DIE("received invalid vector while peeking item", EINVAL); \
        if (vec->size == 0) \
            VEC_DIE("cannot peek an empty vector", EINVAL); \
    \
        return vec->data[vec->size - 1]; \
    } \
    \
    static inline T VEC_FN(name, _at)(const VEC_STRUCT(name) *vec, size_t index) { \
        if (!vec || !vec->data) \
            VEC_DIE("received invalid vector while indexing", EINVAL); \
        if (index >= vec->size) \
            VEC_DIE("index out of bounds", ERANGE); \
    \
        return vec->data[index]; \
    } \
    \
    static inline T VEC_FN(name, _last)(const VEC_STRUCT(name) *vec) { \
        if(vec->size == 0) \
            VEC_DIE("vector is empty", ERANGE); \
        return vec->data[vec->size - 1]; \
    } \
    \
    static inline void VEC_FN(name, _clear)(VEC_STRUCT(name) *vec) { \
        if (!vec) return; \
        vec->size = 0; \
    } \
    \
    static inline void VEC_FN(name, _destroy)(VEC_STRUCT(name) *vec) { \
        if (!vec) return; \
        free(vec->data); \
        vec->data = nullptr; \
        vec->size = 0; \
        vec->capacity = 0; \
    } \
    \
    static inline T *VEC_FN(name, _unwrap)(VEC_STRUCT(name) *vec) { \
        if (!vec || !vec->data) return nullptr; \
    \
        T *data = vec->data; \
        if (vec->size == 0) { \
            free(data); \
            data = nullptr; \
        } else { \
            T *temp = realloc(data, vec->size * sizeof(T)); \
            if (temp != nullptr) data = temp; \
        } \
    \
        *vec = (VEC_STRUCT(name)) { \
            .data = nullptr, \
            .size = 0, \
            .capacity = 0 \
        }; \
        return data; \
    }

#endif // VEC_H