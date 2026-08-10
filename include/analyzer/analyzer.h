#ifndef ANALYZER_H
#define ANALYZER_H

#include <stddef.h>

#include "scope.h"
#include "mem/arena.h"
#include "parser/parser.h"
#include "analyzer/statement.h"

STRMAP_DEFINE_H(type_t*, type)
VEC_DEFINE_H(typed_stmt_t*, t_stmt)

typedef struct {
    fe_err_t *errs;
    size_t err_count;

    typed_stmt_t **stmts;
    size_t stmt_count;

    arena_t arena;
} analyzer_result_t;

typedef struct {
    err_vec_t err_vec;
    type_map_t type_map;
    scope_t *scope;

    arena_t arena;
} analyzer_t;

static analyzer_t analyzer;

analyzer_result_t analyze(stmt_t **statements, size_t statement_count);

#endif // ANALYZER_H
