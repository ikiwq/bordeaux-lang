#ifndef SCOPE_H
#define SCOPE_H

#include "statement.h"
#include "types.h"
#include "dsa/strmap.h"

typedef enum {
    OBJ_VARIABLE,
    OBJ_FUNCTION,
    OBJ_TYPE,
} obj_kind_t;

typedef struct {
    obj_kind_t kind;
    const char *name;

    struct {
        struct {
            type_t *type;
            bool defined;
        } variable;

        struct {
            typed_fun_param_t *params;
            size_t param_count;
            type_t *return_type;
        } function;

        type_t *type;
    } as;
} obj_t;

STRMAP_DEFINE_H(obj_t, obj)

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUN,
    SCOPE_WHILE,
    SCOPE_FOR
} scope_kind_t;

typedef struct scope scope_t;

struct scope {
    scope_t *parent;
    scope_kind_t kind;

    obj_map_t obj_map;

    type_t *return_type;
};

#endif // SCOPE_H
