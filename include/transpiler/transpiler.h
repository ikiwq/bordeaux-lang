#ifndef BORDEAUX_TRANSPILER_H
#define BORDEAUX_TRANSPILER_H

#include "parser/stmt.h"

#include <stdio.h>

void transpile(stmt_t **stmts, size_t stmt_count);

#endif // BORDEAUX_TRANSPILER_H
