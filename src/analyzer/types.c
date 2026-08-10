#include "analyzer/types.h"

bool is_integral_type(const type_t *type) {
    if (type == nullptr) return false;
    return type->kind == TY_INT || type->kind == TY_UINT;
}
