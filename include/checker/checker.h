#ifndef BORDEAUX_ANALYZER_H
#define BORDEAUX_ANALYZER_H

#include "checker/scope.h"
#include "parser/stmt.h"

typedef struct {
    scope_t *scope;
    bool has_error;
} checker_t;

extern checker_t checker;

bool check(stmt_t **stmts, size_t stmt_count);

#endif // BORDEAUX_ANALYZER_H
