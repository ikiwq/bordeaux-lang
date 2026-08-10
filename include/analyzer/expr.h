#ifndef ANALYZER_EXPR_H
#define ANALYZER_EXPR_H

#include "parser/expr.h"
#include "analyzer/types.h"

typedef enum {
    TY_EXPR_IDENTIFIER,
    TY_EXPR_LITERAL,
    TY_EXPR_INDEX,
    TY_EXPR_UNARY,
    TY_EXPR_BINARY,
    TY_EXPR_GROUP,
    TY_EXPR_CALL
} typed_expr_kind_t;

typedef struct typed_expr typed_expr_t;

struct typed_expr {
    typed_expr_kind_t kind;
    span_t span;
    type_t *type;

    union {
        token_t identifier;
        literal_t literal;

        struct {
            typed_expr_t **elements;
            size_t element_count;
        } array_literal;

        struct {
            typed_expr_t *array, *index;
        } index;

        struct {
            token_t op;
            typed_expr_t *right;
        } unary;

        struct {
            typed_expr_t *left;
            token_t op;
            typed_expr_t *right;
        } binary;

        struct {
            typed_expr_t *inner;
        } group;

        struct {
            typed_expr_t *callee;
            typed_expr_t **args;
            size_t args_count;
        } call;
    } as;
};

#endif // ANALYZER_EXPR_H
