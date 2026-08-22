#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include "mem/arena.h"

typedef struct {
    err_avec_t *err_avec;
    token_avec_t *token_avec;

    arena_t *arena;
} scanner_result_t;

typedef struct {
    src_file_t src_file;

    const char *src;
    const char *start;
    const char *current;

    token_avec_t *token_avec;
    err_avec_t *err_avec;

    arena_t *arena;
} scanner_t;

scanner_result_t scan(src_file_t);

#endif // SCANNER_H
