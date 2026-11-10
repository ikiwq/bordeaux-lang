#include "scanner/scanner.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "scanner/token.h"
#include "char.h"
#include "str.h"

static scanner_t scanner;

static void skip_whitespace();

static token_t scan_string();
static token_t scan_number();
static token_t scan_char();
static token_t scan_identifier();

static span_t make_span();
static token_t make_token(token_kind_t token_kind);
static token_t make_err(const char *fmt, ...);

static bool can_precede_equals(char c);

static bool has_more();
static char peek();
static char peek_next();
static char advance();

void scanner_init(const char *src) {
    scanner.src = src;
    scanner.start = src;
    scanner.current = src;
}

token_t scan_token(void) {
    skip_whitespace();
    scanner.start = scanner.current;

    if (!has_more()) return make_token(TOKEN_EOF);

    const char c = advance();

    if (can_precede_equals(c) && peek() == '=') {
        advance();
        switch (c) {
            case '>': return make_token(TOKEN_GREATER_EQUAL);
            case '<': return make_token(TOKEN_LESS_EQUAL);
            case '=': return make_token(TOKEN_EQUAL_EQUAL);
            case '!': return make_token(TOKEN_BANG_EQUAL);
            default: exit(-1);
        }
    }

    if (c == '&') {
        if (peek() == '&') {
            advance();
            return make_token(TOKEN_AND);
        }
        return make_token(TOKEN_AMPERSAND);
    }
    if (c == '|') {
        if (peek() == '|') {
            advance();
            return make_token(TOKEN_OR);
        }
        return make_token(TOKEN_PIPE);
    }
    if (c == '-') {
        if (peek() == '>') {
            advance();
            return make_token(TOKEN_ARROW);
        }
        return make_token(TOKEN_MINUS);
    }

    if (c == '"') return scan_string();
    if (c == '\'') return scan_char();
    if (is_numeric(c)) return scan_number();
    if (is_alpha(c)) return scan_identifier();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '[': return make_token(TOKEN_LEFT_BRACKET);
        case ']': return make_token(TOKEN_RIGHT_BRACKET);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '+': return make_token(TOKEN_PLUS);
        case ':': return make_token(TOKEN_COLON);
        case ';': return make_token(TOKEN_SEMICOLON);
        case '/': return make_token(TOKEN_SLASH);
        case '*': return make_token(TOKEN_STAR);
        case '!': return make_token(TOKEN_BANG);
        case '=': return make_token(TOKEN_EQUAL);
        case '>': return make_token(TOKEN_GREATER);
        case '<': return make_token(TOKEN_LESS);
        case '%': return make_token(TOKEN_PERCENT);
        default: return make_err("Unknown character %c", c);
    }
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

static token_t scan_string() {
    while (has_more() && peek() != '"') {
        advance();
    }

    if (peek() != '"') {
        return make_err("Unterminated string");
    }

    scanner.start += 1; // Trims starting '"'
    const token_t token = make_token(TOKEN_STRING);
    advance(); // Consume trailing '"'
    return token;
}

static token_t scan_number() {
    while (is_numeric(peek())) advance();

    bool is_float = false;
    if (peek() == '.' && is_numeric(peek_next())) {
        is_float = true;
        advance();
        while (is_numeric(peek())) advance();
    }

    if (peek() == '.' && is_numeric(peek_next())) {
        while (is_numeric(peek()) || peek() == '.') advance();
        return make_err("Invalid number '%.*s'",
            (int)(scanner.current - scanner.start), scanner.start);
    }

    return make_token(is_float ? TOKEN_FLOAT : TOKEN_INTEGER);
}

static token_t scan_char() {
    scanner.start += 1; // Trims starting '
    if (has_more()) advance();
    if (peek() != '\'') {
        return make_err("Invalid character");
    }
    const token_t token = make_token(TOKEN_CHAR);
    advance(); // Consume trailing '
    return token;
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
        case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': return check_keyword(1, 7, "ontinue", TOKEN_CONTINUE);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'l': return check_keyword(1, 2, "et", TOKEN_LET);
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

static token_t scan_identifier() {
    while (is_alpha(peek()) || is_numeric(peek())) advance();
    return make_token(identifier_kind());
}

static span_t make_span() {
    return (span_t) {
        .start = (size_t) (scanner.start - scanner.src),
        .end = (size_t) (scanner.current - scanner.src)
    };
}

static token_t make_token(const token_kind_t token_kind) {
    token_t token = (token_t) {
        .kind = token_kind,
    };

    if (token_kind == TOKEN_INTEGER) {
        errno = 0;
        const uint64_t val = strtoull(scanner.start, nullptr, 10);
        if (errno == ERANGE || val > INT64_MAX) {
            return make_err("Integer out of range");
        }
        token.attr.num_val = val;
    } else if (token_kind == TOKEN_FLOAT) {
        errno = 0;
        const double val = strtod(scanner.start, nullptr);
        if (errno == ERANGE) {
            return make_err("Float literal out of range");
        }
        token.attr.float_val = val;
    } else if (token_kind == TOKEN_CHAR) {
        token.attr.char_val = (unsigned char) *scanner.start;
    } else if (token_kind == TOKEN_FALSE) {
        token.attr.bool_val = false;
    } else if (token_kind == TOKEN_TRUE) {
        token.attr.bool_val = true;
    }

    span_t span = make_span();
    const char *ptr = scanner.start;
    size_t len = span.end - span.start;

    token.span = span;
    token.lexeme = STR(ptr, len);

    return token;
}

static token_t make_err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);

    const span_t span = make_span();
    report_err(span, "%s", buf);

    const char *ptr = scanner.start;
    size_t len = span.end - span.start;

    return (token_t) {
        .kind = TOKEN_ERROR,
        .lexeme = STR(ptr , len),
        .span = span
    };
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
    if (!has_more()) return '\0';
    return *(scanner.current + 1);
}

static char advance() {
    return *scanner.current++;
}
