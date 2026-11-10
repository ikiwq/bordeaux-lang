#ifndef BORDEAUX_TYPES_H
#define BORDEAUX_TYPES_H

#include <stddef.h>

#include "common.h"
#include "str.h"

typedef enum {
    TY_INT = 1,
    TY_FLOAT,
    TY_BOOL,
    TY_CHAR,
    TY_STRING,
    TY_FUNCTION,
    TY_VOID,
    TY_ERROR
} type_kind_t;

typedef struct type type_t;

struct type {
    type_kind_t kind;
    str_t lexeme;

    union {
        struct {
            type_t **params;
            size_t param_count;
            type_t *return_type;
        } function;
    };
};

extern type_t ty_int;
extern type_t ty_float;
extern type_t ty_string;
extern type_t ty_bool;
extern type_t ty_char;
extern type_t ty_void;
extern type_t ty_error;

extern type_t *const prims[];
extern const size_t prims_count;

size_t get_type_size(type_t *type);

static type_kind_t comparable_types[] = { TY_INT, TY_FLOAT };
static inline bool is_comparable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(comparable_types); i++) {
        if (type->kind == comparable_types[i]) {
            return true;
        }
    }
    return false;
}

static type_kind_t computable_types[] = { TY_INT, TY_FLOAT };
static inline bool is_computable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(computable_types); i++) {
        if (type->kind == computable_types[i]) {
            return true;
        }
    }
    return false;
}

static type_kind_t negatable_types[] = { TY_INT, TY_FLOAT };
static inline bool is_negatable(const type_t *type) {
    for (size_t i = 0; i < ARRAY_SIZE(negatable_types); i++) {
        if (type->kind == negatable_types[i]) {
            return true;
        }
    }
    return false;
}

#endif // BORDEAUX_TYPES_H
