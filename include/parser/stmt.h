#ifndef BORDEAUX_STMT_H
#define BORDEAUX_STMT_H

#include "parser/expr.h"
#include "scanner/token.h"
#include "types.h"

typedef enum {
    STMT_FUN_DECL,
    STMT_VAR_DECL,
    STMT_IF,
    STMT_FOR,
    STMT_WHILE,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_EXPR
} stmt_kind_t;

typedef struct {
    token_t identifier;
    type_t *type;
} fun_param_t;

#define VEC_TYPE fun_param_t
#define VEC_NAME fun_param
#include "dsa/vec.h"

typedef struct stmt stmt_t;

#define VEC_TYPE stmt_t *
#define VEC_NAME stmt
#include "dsa/vec.h"

struct stmt {
    stmt_kind_t kind;
    span_t span;

    union {
        struct {
            token_t identifier;
            fun_param_vec_t params;
            type_t *return_type;
            stmt_t *body;
        } fun;

        struct {
            token_t identifier;
            type_t *type; // nullptr when the type is inferred from the value
            expr_t *value;
        } var;

        struct {
            expr_t *then_condition;
            stmt_t *then_body;
            stmt_t *else_body;
        } if_;

        struct {
            stmt_t *initializer;
            expr_t *condition;
            expr_t *increment;
            stmt_t *body;
        } for_;

        struct {
            expr_t *condition;
            stmt_t *body;
        } while_;

        struct {
            expr_t *value;
        } return_;

        stmt_vec_t block;

        expr_t *expr;
    };
};

stmt_t *global_statement();

#endif // BORDEAUX_STMT_H
