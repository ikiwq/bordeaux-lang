#include "parser/expr.h"

bool is_assignable(const expr_t *e) {
    switch (e->kind) {
        // TODO: Put struct accessing expression in the future :O
        case EXPR_IDENTIFIER:
        case EXPR_INDEX:
            return true;
        default:
            return false;
    }
}