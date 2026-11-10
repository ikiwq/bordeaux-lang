#ifndef BORDEAUX_PARSER_DEBUG_H
#define BORDEAUX_PARSER_DEBUG_H

#include <stddef.h>

#include "parser/expr.h"
#include "parser/stmt.h"

void print_stmts(stmt_t **stmts, size_t stmt_count);
void print_stmt(const stmt_t *stmt, size_t level);
void print_expr(const expr_t *expr, size_t level);

#endif // BORDEAUX_PARSER_DEBUG_H
