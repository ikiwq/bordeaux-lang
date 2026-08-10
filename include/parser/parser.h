#ifndef PARSER_H
#define PARSER_H

#include "expr.h"
#include "mem/arena.h"

typedef struct {
    token_t *tokens;
    size_t token_count;
    size_t current;

    err_vec_t err_vec;
    arena_t arena;
} parser_t;


typedef struct {
    token_t name;
    expr_t *type;
} fun_parameter_t;

typedef enum {
    STMT_IF,
    STMT_WHILE, STMT_FOR,
    STMT_CONTINUE, STMT_BREAK,
    STMT_VAR_DECLARATION, STMT_FUN_DECLARATION,
    STMT_RETURN,
    STMT_EXPR,
    STMT_BLOCK
} stmt_kind_t;

typedef struct stmt stmt_t;

struct stmt {
    stmt_kind_t kind;
    span_t span;

    union {
        struct {
            expr_t *condition;
            stmt_t *then_branch;
            stmt_t *else_branch;
        } _if;

        struct {
            expr_t *condition;
            stmt_t *body;
        } _while;

        struct {
            stmt_t *init;
            expr_t *condition;
            expr_t *increment;
            stmt_t *body;
        } _for;

        struct {
            token_t identifier;
            expr_t *type;
            expr_t *initializer;
        } var_decl;

        struct {
            token_t identifier;
            fun_parameter_t *params;
            size_t params_count;
            expr_t *return_type;
            stmt_t *body;
        } fun_decl;

        struct {
            token_t keyword;
            expr_t *value;
        } _return;

        struct {
            stmt_t **stmts;
            size_t stmt_count;
        } block;

        expr_t *expr;
    } as;
};

typedef struct {
    fe_err_t *errs;
    size_t err_count;

    stmt_t **stmts;
    size_t stmt_count;

    arena_t arena;
} parser_result_t;

parser_result_t parse(token_t *tokens, size_t token_count);

// Print statements and expression in a human readable format
void print_statements(stmt_t **statements, size_t statement_count);

#endif // PARSER_H
