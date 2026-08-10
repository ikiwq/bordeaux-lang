#ifndef TYPES_H
#define TYPES_H
#include <stddef.h>

typedef enum {
    INT64,
    INT32,
    INT16,
    INT8
} int_kind_t;

typedef enum {
    FLOAT64,
    FLOAT32
} float_kind_t;

typedef enum {
    TY_INT,
    TY_UINT,
    TY_FLOAT,
    TY_BOOL,
    TY_CHAR,
    TY_STRING,
    TY_REFERENCE,
    TY_POINTER,
    TY_ARRAY,
    TY_SLICE,
    TY_ALIAS,
    TY_STRUCT,
    TY_VOID,
    TY_ERROR
} type_kind_t;

typedef struct type type_t;

struct type {
    type_kind_t kind;
    const char *lexeme;

    union {
        struct {
            int_kind_t kind;
        } _int;

        struct {
            float_kind_t kind;
        } _float;

        struct {
            type_t *element;
            size_t len;
        } array;

        struct {
            type_t *element;
        } slice;

        struct {
            type_t *inner;
        } reference;

        struct {
            type_t *inner;
        } pointer;

        type_t *alias;
    } as;
};

bool is_integral_type(const type_t *t);

#endif // TYPES_H
