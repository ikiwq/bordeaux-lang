#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

#include "scanner/token.h"

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
    TY_FUNCTION,
    TY_ALIAS,
    TY_STRUCT,
    TY_VOID,
    TY_ERROR
} type_kind_t;

typedef struct type type_t;

typedef struct {
    token_t name;
    type_t *type;
} tfunparam_t;


#define AVEC_TYPE tfunparam_t
#define AVEC_NAME tfunparam
#include "dsa/arena_vec.h"

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

        struct {
            tfunparam_avec_t *params;
            type_t *return_type;
        } function;

        type_t *alias;
    } as;
};

// A couple of words on ...this. Type checking is considerably hard if done in a not
// so good way. Suppose for example that you have this sequence of expressions:
//
// let my_p: ***int64 = some_address;
// let my_other_p: ***int64 = some_address;
// my_p == my_other_p
//
// When comparing the type expressions of a and b you could recursively check the types...
// or simply have a singleton of every type. So, each type would definitively have one and only memory address.
// At this point, comparing two types becomes trivial: just compare the two memory addresses.
//
// For type contructors (e.g. arrays, pointers, references), it's trickier because there are
// an infinite amount of combinations that we can't pre-allocate.
//
// We can instead have a hashmap where we dynamically put new type constructors and then retrieve it when necessary.
// The latter won't depend on the scope, since a type constructor relies on type expressions, which are already interned in the
// corresponding scope
//
static type_t ty_int8    = { .lexeme = "int8",    .kind = TY_INT,   .as._int.kind   = INT8    };
static type_t ty_int16   = { .lexeme = "int16",   .kind = TY_INT,   .as._int.kind   = INT16   };
static type_t ty_int32   = { .lexeme = "int32",   .kind = TY_INT,   .as._int.kind   = INT32   };
static type_t ty_int64   = { .lexeme = "int64",   .kind = TY_INT,   .as._int.kind   = INT64   };
static type_t ty_uint8   = { .lexeme = "uint8",   .kind = TY_UINT,  .as._int.kind   = INT8    };
static type_t ty_uint16  = { .lexeme = "uint16",  .kind = TY_UINT,  .as._int.kind   = INT16   };
static type_t ty_uint32  = { .lexeme = "uint32",  .kind = TY_UINT,  .as._int.kind   = INT32   };
static type_t ty_uint64  = { .lexeme = "uint64",  .kind = TY_UINT,  .as._int.kind   = INT64   };
static type_t ty_float32 = { .lexeme = "float32", .kind = TY_FLOAT, .as._float.kind = FLOAT32 };
static type_t ty_float64 = { .lexeme = "float64", .kind = TY_FLOAT, .as._float.kind = FLOAT64 };
static type_t ty_string  = { .lexeme = "string",  .kind = TY_STRING };
static type_t ty_bool    = { .lexeme = "bool",    .kind = TY_BOOL  };
static type_t ty_char    = { .lexeme = "char",    .kind = TY_CHAR  };
static type_t ty_void    = { .lexeme = "void",    .kind = TY_VOID  };
static type_t ty_error   = { .lexeme = "error",   .kind = TY_ERROR };

static type_t *const prims[] = {
    &ty_int8,
    &ty_int16,
    &ty_int32,
    &ty_int64,
    &ty_uint8,
    &ty_uint16,
    &ty_uint32,
    &ty_uint64,
    &ty_float32,
    &ty_float64,
    &ty_string,
    &ty_bool,
    &ty_char,
    &ty_void,
    &ty_error,
};

static inline bool is_integral_type(const type_t* t) {
    if (t == nullptr) return false;
    return t->kind == TY_INT || t->kind == TY_UINT;
}

#endif // TYPES_H
