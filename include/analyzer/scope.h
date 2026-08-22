#ifndef SCOPE_H
#define SCOPE_H

#include "types.h"
#include "dsa/strmap.h"

typedef enum {
    OBJ_ANY = 0,
    OBJ_VARIABLE,
    OBJ_FUNCTION,
    OBJ_TYPE,

    OBJ_UNKNOWN = -1
} obj_kind_t;

typedef struct {
    obj_kind_t kind;
    const char *name;
    bool defined;
    type_t *type;
} obj_t;

#define STRMAP_TYPE obj_t
#define STRMAP_NAME obj
#include "dsa/strmap.h"

typedef enum {
    ASCOPE_GLOBAL,
    ASCOPE_FUN,
    ASCOPE_WHILE,
    ASCOPE_FOR
} ascope_kind_t;

// analyzer scope
typedef struct ascope ascope_t;

struct ascope {
    ascope_kind_t kind;
    ascope_t *parent;
    obj_map_t *obj_map;

    type_t *return_type;
};

#endif // SCOPE_H
