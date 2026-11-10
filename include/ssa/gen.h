#ifndef BORDEAUX_SSA_GEN_H
#define BORDEAUX_SSA_GEN_H

#include "parser/stmt.h"
#include "ssa/quad.h"
#include "mem/arena.h"

typedef struct {
    arena_t *arena;

    block_t *current_block;
    size_t current_block_id;

    size_t current_expr_id;
} ssa_generator_t;

typedef struct {
    block_t **blocks;
    size_t block_count;
} gen_result_t;

gen_result_t gen_ssa(stmt_t **stmts, size_t stmt_count);

#endif // BORDEAUX_SSA_GEN_H
