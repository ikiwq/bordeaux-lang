#ifndef ANALYZER_EXPR_H
#define ANALYZER_EXPR_H

#include "parser/expr.h"
#include "analyzer/types.h"

typedef enum {
    TEXPR_IDENTIFIER,
    TEXPR_LITERAL,
    TEXPR_INDEX,
    TEXPR_UNARY,
    TEXPR_BINARY,
    TEXPR_GROUP,
    TEXPR_CALL
} texpr_kind_t;

typedef struct texpr texpr_t;

#define AVEC_TYPE texpr_t*
#define AVEC_NAME texpr
#include "dsa/arena_vec.h"

struct texpr {
    texpr_kind_t kind;
    span_t span;
    type_t *type;

    union {
        token_t identifier;
        literal_t literal;

        texpr_avec_t array_literal;

        struct {
            texpr_t *array, *index;
        } index;

        struct {
            token_t op;
            texpr_t *right;
        } unary;

        struct {
            texpr_t *left;
            token_t op;
            texpr_t *right;
        } binary;

        struct {
            texpr_t *inner;
        } group;

        struct {
            texpr_t *callee;
            texpr_avec_t *args;
        } call;
    } as;
};

#endif // ANALYZER_EXPR_H
