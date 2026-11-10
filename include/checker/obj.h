#ifndef BORDEAUX_OBJ_H
#define BORDEAUX_OBJ_H

#include "parser/stmt.h"
#include "str.h"
#include "types.h"

typedef enum {
    OBJ_VAR,
    OBJ_FUN,
    OBJ_TYPE
} obj_kind_t;

typedef struct {
    obj_kind_t obj_kind;
    str_t lexeme;
    bool defined;

    union {
        struct {
            type_t *type;
        } var;

        struct {
            fun_param_t **params;
            size_t param_count;
            type_t *return_type;
        } fun;
    };
} obj_t;

#endif // BORDEAUX_OBJ_H
