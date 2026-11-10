#ifndef BORDEAUX_DECL_H
#define BORDEAUX_DECL_H

#include <stddef.h>

#include "types.h"

// Functions that are available without being declared in the source
typedef struct {
    const char *name;
    type_t *const *params;
    size_t param_count;
    type_t *return_type;
} builtin_fun_t;

extern const builtin_fun_t builtin_funs[];
extern const size_t builtin_funs_count;

#endif // BORDEAUX_DECL_H
