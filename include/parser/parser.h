#ifndef PARSER_H
#define PARSER_H

#include "expr.h"
#include "mem/arena.h"

typedef struct {
    token_avec_t *token_avec;
    size_t current;

    err_avec_t *err_avec;
    arena_t *arena;
} parser_t;

typedef struct {
    token_t name;
    expr_t *type;
} fnparam_t;

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

#define AVEC_TYPE fnparam_t
#define AVEC_NAME fnparam
#include "dsa/arena_vec.h"

#define AVEC_TYPE stmt_t*
#define AVEC_NAME stmt
#include "dsa/arena_vec.h"

struct stmt {
    stmt_kind_t kind;
    span_t span;

    union {
        struct {
            stmt_avec_t *then_branches;
            expr_avec_t *conditions;
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
            fnparam_avec_t *params;
            expr_t *return_type;
            stmt_t *body;
        } fun_decl;

        struct {
            token_t keyword;
            expr_t *value;
        } _return;

        stmt_avec_t *block;

        expr_t *expr;
    } as;
};

typedef struct {
    err_avec_t *err_avec;
    stmt_avec_t *stmt_avec;

    arena_t *arena;
} parser_result_t;

parser_result_t parse(token_avec_t *token_avec);

void print_statements(stmt_avec_t *stmt_avec);

#endif // PARSER_H
