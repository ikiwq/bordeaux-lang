#include "scanner/scanner.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "dsa/strutils.h"

static scanner_t scanner;

static void scanner_init(src_file_t *src_file);
static void skip_whitespace();

static void scan_string();
static void scan_number();
static void scan_char();
static void scan_identifier();

static span_t make_span();
static void make_token(token_kind_t token_kind);
static void make_err(const char *fmt, ...);

static bool can_precede_equals(char c);

static bool has_more();
static char peek();
static char peek_next();
static char advance();

scanner_result_t scan(src_file_t *src_file) {
    scanner_init(src_file);

    while (has_more()) {
        skip_whitespace();
        if (!has_more()) break;

        scanner.start = scanner.current;
        const char c = advance();

        if (can_precede_equals(c) && peek() == '=') {
            advance();
            switch (c) {
                case '>': make_token(TOKEN_GREATER_EQUAL); break;
                case '<': make_token(TOKEN_LESS_EQUAL); break;
                case '=': make_token(TOKEN_EQUAL_EQUAL); break;
                case '!': make_token(TOKEN_BANG_EQUAL); break;
                default: exit(-1);
            }

            continue;
        }

        if (c == '&') {
            if (peek() == '&') {
                advance();
                make_token(TOKEN_AND);
                continue;
            }
            make_token(TOKEN_AMPERSAND);
            continue;
        }
        if (c == '|') {
            if (peek() == '|') {
                advance();
                make_token(TOKEN_OR);
                continue;
            }
            make_token(TOKEN_PIPE);
            continue;
        }

        if (c == '"') {
            scan_string();
            continue;
        }

        if (c == '\'') {
            scan_char();
            continue;
        }

        if (is_numeric(c)) {
            scan_number();
            continue;
        }

        if (is_alpha(c)) {
            scan_identifier();
            continue;
        }

        switch (c) {
            case '(': make_token(TOKEN_LEFT_PAREN); break;
            case ')': make_token(TOKEN_RIGHT_PAREN); break;
            case '[': make_token(TOKEN_LEFT_BRACKET); break;
            case ']': make_token(TOKEN_RIGHT_BRACKET); break;
            case '{': make_token(TOKEN_LEFT_BRACE); break;
            case '}': make_token(TOKEN_RIGHT_BRACE); break;
            case ',': make_token(TOKEN_COMMA); break;
            case '.': make_token(TOKEN_DOT); break;
            case '-': make_token(TOKEN_MINUS); break;
            case '+': make_token(TOKEN_PLUS); break;
            case ':': make_token(TOKEN_COLON); break;
            case ';': make_token(TOKEN_SEMICOLON); break;
            case '/': make_token(TOKEN_SLASH); break;
            case '*': make_token(TOKEN_STAR); break;
            case '!': make_token(TOKEN_BANG); break;
            case '=': make_token(TOKEN_EQUAL); break;
            case '>': make_token(TOKEN_GREATER); break;
            case '<': make_token(TOKEN_LESS); break;
            case '%': make_token(TOKEN_MODULO); break;
            default: make_err("Unknown character %c", c);
        }
    }

    make_token(TOKEN_EOF);

    const size_t token_count = scanner.t_vec.size;
    token_t *tokens =
        arena_copy(&scanner.arena, scanner.t_vec.data, token_count * sizeof *tokens);
    token_vec_destroy(&scanner.t_vec);

    const size_t errs_count = scanner.err_vec.size;
    fe_err_t *errs =
        arena_copy(&scanner.arena, scanner.err_vec.data, errs_count * sizeof *errs);
    err_vec_destroy(&scanner.err_vec);

    const scanner_result_t res = (scanner_result_t) {
        .err_count = errs_count,
        .errs = errs,
        .token_count = token_count,
        .tokens = tokens,
        .arena = scanner.arena
    };

    // Ensure that the arena is no longer used in this scope.
    // The ownership is passed through the return of the result
    scanner.arena = (arena_t){0};

    return res;
}

static void scanner_init(src_file_t *src_file) {
    scanner.src_file = src_file;
    line_vec_push(&src_file->line_vec, 0);

    scanner.src = src_file->src;
    scanner.start = src_file->src;
    scanner.current = src_file->src;

    scanner.t_vec = token_vec_init();
    scanner.err_vec = err_vec_init();

    scanner.arena = arena_make(1u << 28);
}

static void skip_whitespace() {
    for (;;) {
        switch (peek()) {
            case ' ': case '\n': case '\r': case '\t':
                advance();
                break;
            case '/':
                if (peek_next() != '/') return;
                while (peek() != '\n' && has_more()) advance();
                break;
            default:
                return;
        }
    }
}

static void scan_string() {
    while (has_more() && *scanner.current != '"') {
        advance();
    }

    if (*scanner.current != '"') {
        make_err("Unterminated string");
        return;
    }

    scanner.start += 1; // Trims starting '"'
    make_token(TOKEN_STRING);
    advance(); // Consume trailing '"'
}

static void scan_number() {
    while (is_numeric(peek())) advance();

    bool is_float = false;
    if (peek() == '.' && is_numeric(peek_next())) {
        is_float = true;
        advance();
        while (is_numeric(peek())) advance();
    }

    if (peek() == '.' && is_numeric(peek_next())) {
        while (is_numeric(peek()) || peek() == '.') advance();
        make_err("Invalid number '%.*s'",
            (int)(scanner.current - scanner.start), scanner.start);
        return;
    }

    make_token(is_float ? TOKEN_FLOAT : TOKEN_INTEGER);
}

static void scan_char() {
    scanner.start += 1; // Trims starting '
    advance(); // Will be caught by make_token
    if (*scanner.current != '\'') {
        make_err("Invalid character");
    }
    make_token(TOKEN_CHAR);
    advance(); // Consume trailing '
}

static token_kind_t check_keyword(const size_t offset, const size_t rest_len, const char *rest, const token_kind_t kind) {
   const size_t len = scanner.current - scanner.start;
    if (len == offset + rest_len && memcmp(scanner.start + offset, rest, rest_len) == 0) {
        return kind;
    }

    return TOKEN_IDENTIFIER;
}

static token_kind_t identifier_kind() {
    switch (scanner.start[0]) {
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': return check_keyword(1, 7, "ontinue", TOKEN_CONTINUE);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'l': return check_keyword(1, 2, "et", TOKEN_LET);
        case 's': return check_keyword(1, 5, "truct", TOKEN_STRUCT);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 't': return check_keyword(1, 3, "rue", TOKEN_TRUE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    default: break;
                }
            }
            break;
        default: break;
    }
    return TOKEN_IDENTIFIER;
}

static void scan_identifier() {
    while (is_alpha(peek()) || is_numeric(peek())) advance();
    make_token(identifier_kind());
}

static span_t make_span() {
    return (span_t) {
        .src = scanner.start,
        .length = (size_t) (scanner.current - scanner.start),
        .start = (size_t) (scanner.start - scanner.src),
        .end = (size_t) (scanner.current - scanner.src)
    };
}

static void make_token(const token_kind_t token_kind) {
    token_t token = (token_t) {
        .kind = token_kind,
    };

    if (token_kind == TOKEN_INTEGER) {
        errno = 0;
        const uint64_t val = strtoull(scanner.start, nullptr, 10);
        if (errno == ERANGE) {
            make_err("Integer out of range");
            return;
        }
        token.attr.num_val = val;
    } else if (token_kind == TOKEN_FLOAT) {
        errno = 0;
        const double val = strtod(scanner.start, nullptr);
        if (errno == ERANGE) {
            make_err("Float literal out of range");
            return;
        }
        token.attr.float_val = val;
    } else if (token_kind == TOKEN_CHAR) {
        token.attr.char_val = (unsigned char) *scanner.start;
    }

    token.span = make_span();

    token_vec_push(&scanner.t_vec, token);
}

static void make_err(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0) return;

    char *buf = arena_alloc(&scanner.arena, (size_t)len + 1);
    if (!buf) return;

    va_start(args, fmt);
    vsnprintf(buf, (size_t)len + 1, fmt, args);
    va_end(args);

    err_vec_push(&scanner.err_vec, (fe_err_t){
        .span = make_span(),
        .err  = buf,
    });
}

static bool can_precede_equals(const char c) {
    return c == '=' || c == '<' || c == '>' || c == '!';
}

static bool has_more() {
    return *scanner.current != '\0';
}

static char peek() {
    return *scanner.current;
}

static char peek_next() {
    return *(scanner.current + 1);
}

static char advance() {
    const char c = *scanner.current++;
    if (c == '\n')
        line_vec_push(&scanner.src_file->line_vec, (size_t)(scanner.current - scanner.src));
    return c;
}
