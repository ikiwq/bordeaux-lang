#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include "mem/arena.h"

#include "dsa/vector.h"
VEC_DEFINE_H(token_t, token)
VEC_DEFINE_H(fe_err_t, err)

typedef struct {
    token_t *tokens;
    size_t token_count;

    fe_err_t *errs;
    size_t err_count;

    arena_t arena;
} scanner_result_t;

typedef struct {
    src_file_t *src_file;

    const char *src;
    const char *start;
    const char *current;

    token_vec_t t_vec;
    err_vec_t err_vec;

    arena_t arena;
} scanner_t;

scanner_result_t scan(src_file_t*);

#endif // SCANNER_H
