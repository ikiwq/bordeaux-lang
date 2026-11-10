#include "parser/parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem/pvec.h"
#include "scanner/scanner.h"
#include "parser/stmt.h"

parser_t parser;

static void init_parser(const char *src);
static void panic_mode();

parse_result_t parse(const char *src, arena_t *arena) {
    scanner_init(src);

    parser.arena = arena;
    advance(); // Load the first token

    pvec_t *pvec = pvec_init();

    bool had_error = false;
    while (has_more()) {
        stmt_t *stmt = global_statement();
        if(!stmt) {
            had_error = true;
            panic_mode();
            continue;
        }
        pvec_push(pvec, stmt);
    }

    size_t stmt_count = pvec->size;
    stmt_t **stmts = pvec_unwrap(pvec, arena);

    return (parse_result_t) {
        .success = !had_error,
        .stmts = stmts,
        .stmt_count = stmt_count
    };
}

static void init_parser(const char *src) {
}

static token_kind_t sync_tokens[] = {
    TOKEN_LET, TOKEN_FUN,
    TOKEN_IF, TOKEN_WHILE, TOKEN_FOR,
    TOKEN_RETURN, TOKEN_BREAK, TOKEN_CONTINUE,
};

static void panic_mode() {
    while (has_more()) {
        const token_kind_t next_kind = peek().kind;
        if (next_kind == TOKEN_SEMICOLON) {
            advance();
            return;
        }
        for (size_t i = 0; i < ARRAY_SIZE(sync_tokens); i++) {
            if (next_kind == sync_tokens[i]) {
                return;
            }
        }
        advance();
    }
}

bool has_more() {
    return parser.current.kind != TOKEN_EOF;
}

token_t advance() {
    parser.previous = parser.current;

    if (parser.previous.kind == TOKEN_EOF) return parser.previous;

    do {
        parser.current = scan_token();
    } while (parser.current.kind == TOKEN_ERROR);

    return parser.previous;
}

token_t peek() {
    return parser.current;
}

token_t expect(const token_kind_t expected) {
    const token_t next = advance();
    if (next.kind != expected) {
        if (next.kind == TOKEN_EOF) {
            report_err(next.span, "Unexpected end of file");
        } else {
            report_err(next.span, "Unexpected '" STR_FMT "'", STR_ARG(next.lexeme));
        }
        return (token_t) {
            .kind = TOKEN_ERROR
        };
    }

    return next;
}

bool matches(const token_kind_t expected) {
    return peek().kind == expected;
}

span_t span_join(const span_t from, const span_t to) {
    return (span_t) {
        .start = from.start,
        .end = to.end
    };
}
