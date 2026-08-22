#ifndef EXPR_H
#define EXPR_H

#include <stdint.h>

#include "scanner/token.h"

// When storing literal during compilation, store always the widest possible type (for simplicity!).
// When doing type checking, hopefully we can narrow the type of the variable down to the
// highest bound and not waste memory during execution
typedef enum {
    LIT_INTEGER,
    LIT_FLOAT,
    LIT_STRING,
    LIT_CHAR,
    LIT_BOOL,
} literal_kind_t;

typedef struct {
    literal_kind_t kind;
    union {
        uint64_t _int;
        double _float;
        uint8_t _char;
        bool _bool;

        struct {
            const char *start;
            size_t length;
        } string;
    };
} literal_t;

typedef enum {
    EXPR_IDENTIFIER,
    EXPR_LITERAL,
    EXPR_ARR_LITERAL,
    EXPR_INDEX,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_GROUP,
    EXPR_CALL,
    EXPR_TYPE_NAME,
    EXPR_ARR_TYPE,
    EXPR_SLICE_TYPE,
    EXPR_REFERENCE_TYPE,
    EXPR_POINTER_TYPE
} expr_kind_t;

typedef struct expr expr_t;

#define AVEC_TYPE expr_t*
#define AVEC_NAME expr
#include "dsa/arena_vec.h"

struct expr {
    expr_kind_t kind;
    span_t span;

    union {
        token_t identifier;

        literal_t literal;

        // [value, value, value]
        expr_avec_t *array_literal;

        // array[index]
        struct {
            expr_t *array, *index;
        } index;


        // op right
        struct {
            token_t op;
            expr_t *right;
        } unary;

        // left op right
        struct {
            expr_t *left;
            token_t op;
            expr_t *right;
        } binary;

        //( inner )
        struct {
            token_t left_paren;
            expr_t *inner;
            token_t right_paren;
        } group;

        // callee(args), with args_count = len(args)
        struct {
            expr_t *callee;
            expr_avec_t *args;
        } call;


        // Types during parsing are considered syntactic expressions and therefore treated
        // as expressions and not as a separate data type.
        // A basic type expression like "int64" or "float64" (or even a struct name) is considered a type name.
        // Type constructors are type expressions that apply a constructor to other type expressions.

        token_t type_name;

        // [len]element
        struct {
            expr_t *len;
            expr_t *element;
        } array_type;

        // []element
        struct {
            expr_t *element;
        } slice_type;

        // &element
        expr_t *reference_type;

        // *element
        expr_t *pointer_type;
    } as;
};

bool is_assignable(const expr_t *e);

#endif
