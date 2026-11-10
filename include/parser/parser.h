#ifndef BORDEAUX_PARSER_H
#define BORDEAUX_PARSER_H

#include "mem/arena.h"
#include "parser/stmt.h"
#include "scanner/token.h"

typedef struct {
    token_t current;
    token_t previous;
    arena_t *arena;
} parser_t;

extern parser_t parser;

typedef struct {
    bool success;
    stmt_t **stmts;
    size_t stmt_count;
} parse_result_t;

parse_result_t parse(const char *src, arena_t *arena);

bool has_more();
token_t advance();
token_t peek();
token_t expect(const token_kind_t expected);
bool matches(const token_kind_t expected);
span_t span_join(const span_t from, const span_t to);


#endif // BORDEAUX_PARSER_H
