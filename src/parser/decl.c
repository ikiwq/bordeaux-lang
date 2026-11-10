#include "parser/decl.h"
#include "common.h"
#include "types.h"

static type_t *const print_params[] = { &ty_string };
static type_t *const intstr_params[] = { &ty_int };

const builtin_fun_t builtin_funs[] = {
    {
        .name = "print",
        .params = print_params,
        .param_count = ARRAY_SIZE(print_params),
        .return_type = &ty_void
    },
    {
        .name = "intstr",
        .params = intstr_params,
        .param_count = ARRAY_SIZE(intstr_params),
        .return_type = &ty_string
    }
};

const size_t builtin_funs_count = ARRAY_SIZE(builtin_funs);
