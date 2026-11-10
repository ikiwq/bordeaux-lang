#include "mem/arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t align_size(size_t n, size_t a) {
    assert(a != 0 && (a & (a - 1)) == 0);
    return (n + a - 1) & ~(a - 1);
}

arena_t *arena_make(const size_t size) {
    assert(size > 0);

    arena_t a;

#ifdef _WIN32
    void *p = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    if (p == nullptr) {
        fprintf(stderr, "Cannot allocate memory for arena");
        exit(EXIT_FAILURE);
    }
    a.committed = 0;
#else
    void *p = mmap(NULL, size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                   -1, 0);
    if (p == MAP_FAILED) {
        fprintf(stderr, "Cannot allocate memory for arena");
        exit(EXIT_FAILURE);
    }
    a.committed = size;
#endif
    a.base_ptr = p;
    a.reserved = size;
    a.used = 0;

    arena_t *a_ptr = arena_alloc(&a, sizeof *a_ptr);
    *a_ptr = (arena_t){
        .committed = a.committed,
        .base_ptr = a.base_ptr,
        .reserved = a.reserved,
        .used = a.used
    };

    return a_ptr;
}

void *arena_alloc(arena_t *a, const size_t size) {
    return arena_alloc_aligned(a, size, ARENA_DEFAULT_ALIGN);
}

void *arena_alloc_aligned(arena_t *a, const size_t size, const size_t align) {
    assert(a != nullptr);
    assert(size > 0);

    const size_t off = align_size(a->used, align);

    if (off > a->reserved || size > a->reserved - off) {
        fprintf(stderr, "Arena out of memory");
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    if (off + size > a->committed) {
        size_t want = align_size(off + size, 64 * 1024);
        if (want > a->reserved) want = a->reserved;
        if (!VirtualAlloc(a->base_ptr + a->committed, want - a->committed,
                          MEM_COMMIT, PAGE_READWRITE)) {
            fprintf(stderr, "bordeaux: cannot commit arena memory\n");
            exit(EXIT_FAILURE);
                          }
        a->committed = want;
    }
#endif

    char *p = a->base_ptr + off;
    a->used = off + size;
    return p;
}

void *arena_copy(arena_t *a, const void *src, const size_t size) {
    if (size == 0) return nullptr;
    void *dst = arena_alloc(a, size);
    memcpy(dst, src, size);
    return dst;
}

void arena_destroy(arena_t *a) {
#ifdef _WIN32
    VirtualFree(a->base_ptr, 0, MEM_RELEASE);
#else
    munmap(a->base_ptr, a->reserved);
#endif
}

void arena_reset(arena_t *a) {
    a->used = 0;
}
