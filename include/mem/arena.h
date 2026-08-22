#ifndef ARENA_H
#define ARENA_H

#define ARENA_DEFAULT_ALIGN 8

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif

typedef struct {
    char* base_ptr;
    size_t reserved;
    size_t committed;
    size_t used;
} arena_t;

arena_t *arena_make(size_t size);
void *arena_alloc(arena_t *a, size_t size);
void *arena_alloc_aligned(arena_t *a, size_t size, size_t align);
void *arena_copy(arena_t *a, const void *src, size_t size);
void arena_destroy(arena_t *a);
void arena_reset(arena_t *a);

#endif // ARENA_H
