#include "codegen/quad.h"

static icg_t icg;

static void init_icg();

static void t_stmt_analyze(const typed_stmt_t *t_stmt);
static quadruple_t t_expr_analyze(const typed_expr_t *t_expr);

static quad_op_kind_t binary_token_op(token_kind_t);

icg_result_t generate_quads(typed_stmt_t **t_stmts, size_t stmt_count) {
    init_icg();
}

static void init_icg() {
    icg.err_vec = err_vec_init();
    icg.quad_vec = quad_vec_init();
}

static void t_stmt_analyze(const typed_stmt_t *t_stmt) {
    if (t_stmt == nullptr) {
        fprintf(stderr, "cannot analyze null statement");
        exit(EINVAL);
    }

    switch (t_stmt->kind) {
        case STMT_BLOCK: {
            for (size_t i = 0; i < t_stmt->as.block.stmt_count; i++) {
                t_stmt_analyze(t_stmt->as.block.stmts[i]);
            }
            return;
        }
        case STMT_EXPR: {
            t_expr_analyze(t_stmt->as.expr);
            return;
        }
        default: {
            fprintf(stderr, "unknown typed statement");
            exit(EINVAL);
        }
    }
}

static quadruple_t t_expr_analyze(const typed_expr_t *t_expr) {
    if (t_expr == nullptr) {
        fprintf(stderr, "cannot analyze null expression");
        exit(EINVAL);
    }

    switch (t_expr->kind) {
        case TY_EXPR_BINARY: {
            const quadruple_t left = t_expr_analyze(t_expr->as.binary.left);
            const quadruple_t right = t_expr_analyze(t_expr->as.binary.right);

            return (quadruple_t) {
                .op = binary_token_op(t_expr->as.binary.op.kind),
                .arg1 = left.result,
                .arg2 = right.result
            };
        }
        default: {
            fprintf(stderr, "unknown typed expression");
            exit(EINVAL);
        }
    }

}


static quad_op_kind_t binary_token_op(token_kind_t t) {
    switch (t) {
        case TOKEN_PLUS: return QUAD_OP_ADD;
        case TOKEN_MINUS: return QUAD_OP_SUBTRACT;
        case TOKEN_SLASH: return QUAD_OP_DIVIDE;
        case TOKEN_STAR: return QUAD_OP_MULTIPLY;
        case TOKEN_AND: return QUAD_OP_AND;
        case TOKEN_OR: return QUAD_OP_OR;
        case TOKEN_EQUAL: return QUAD_OP_COPY;
        default: {
            fprintf(stderr, "cannot map token op %s to quad op", token_kind_to_str(t));
            exit(EINVAL);
        }
    }
}
