#include "ssa/gen.h"
#include "parser/stmt.h"
#include "mem/pvec.h"
#include "ssa/quad.h"

#include <stdlib.h>

static ssa_generator_t ssa_generator;

static block_t *block_make();

static ssa_quad_t *quad_make(op_kind_t op);

gen_result_t gen_ssa(stmt_t **stmts, size_t stmt_count) {
    ssa_generator = (ssa_generator_t) {
        .current_block = nullptr,
        .current_block_id = 0,
        .current_expr_id = 0,
    };

    pvec_t *pvec = pvec_init();

    for(size_t i = 0; i < stmt_count; i++) {
        stmt_t *stmt = stmts[i];
        switch (stmt->kind) {
            case STMT_FUN_DECL: {
                ssa_generator.current_block = block_make();

                for(size_t i = 0; i < stmt->fun.param_count; i++) {
                    fun_param_t *param = stmt->fun.params[i];
                    ssa_quad_t *quad = quad_make(OP_PARAM);

                    put_symbol(ssa_generator.current_block, param->identifier.lexeme, quad);
                }
            }
        }
    }

    return (gen_result_t){
        .block_count = pvec->size,
        .blocks = (block_t**)pvec->items
    };
}

static size_t get_block_id() {
    return ssa_generator.current_block_id++;
}

static block_t *block_make() {
    block_t *block = malloc(sizeof *block);
    block->id = get_block_id();

    return block;
}

static ssa_quad_t *quad_make(op_kind_t op) {
    ssa_quad_t *quad = malloc(sizeof *quad);
    quad->op = op;

    return quad;
}

void put_symbol(block_t *block, str_t key, ssa_quad_t *value) {

}
