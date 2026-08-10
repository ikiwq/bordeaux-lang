#ifndef ANALYZER_STATEMENT_H
#define ANALYZER_STATEMENT_H

#include "parser/parser.h"
#include "analyzer/expr.h"

typedef struct {
    token_t name;
    type_t *type;
} typed_fun_param_t;

typedef struct typed_stmt typed_stmt_t;

struct typed_stmt {
    stmt_kind_t kind;
    span_t span;

    union {
        struct {
            typed_expr_t *condition;
            typed_stmt_t *then_branch;
            typed_stmt_t *else_branch;
        } _if;

        struct {
            typed_expr_t *condition;
            typed_stmt_t *body;
        } _while;

        struct {
            typed_stmt_t *init;
            typed_expr_t *condition;
            typed_expr_t *increment;
            typed_stmt_t *body;
        } _for;

        struct {
            token_t identifier;
            typed_expr_t *initializer;
            type_t *type;
        } var_decl;

        struct {
            token_t identifier;
            typed_fun_param_t *params;
            size_t param_count;
            type_t *return_type;
            typed_stmt_t *body;
        } fun_decl;

        struct {
            typed_expr_t *value;
        } _return;

        struct {
            typed_stmt_t **stmts;
            size_t stmt_count;
        } block;

        typed_expr_t *expr;
    } as;
};

void print_typed_stmts(typed_stmt_t **stmts, size_t stmt_count);

#endif // ANALYZER_STATEMENT_H

