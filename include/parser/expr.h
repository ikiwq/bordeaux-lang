#ifndef BORDEAUX_EXPR_H
#define BORDEAUX_EXPR_H

#include "scanner/token.h"
#include "types.h"

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_PRIMARY,
    EXPR_VAR,
} expr_kind_t;

typedef struct expr expr_t;

#define VEC_TYPE expr_t *
#define VEC_NAME expr
#include "dsa/vec.h"

struct expr {
    uint8_t kind;
    span_t span;
    type_t *type;

    union {
        struct {
            expr_t *left;
            token_t op;
            expr_t *right;
        } binary;

        struct {
            token_t op;
            expr_t *right;
        } unary;

        struct {
            token_t callee;
            expr_vec_t args;
        } call;

        token_t var;
        token_t primary;
    };
};

expr_t *expression();
type_t *expr_type();

#endif // BORDEAUX_EXPR_H
