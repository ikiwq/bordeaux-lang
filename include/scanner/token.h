#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

#include "common.h"

typedef enum {
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_AMPERSAND, TOKEN_PIPE,

    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_MODULO, TOKEN_ARROW,

    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_CHAR, TOKEN_FALSE, TOKEN_TRUE,

    TOKEN_STRUCT, TOKEN_LET, TOKEN_FUN,
    TOKEN_IF, TOKEN_ELSE,
    TOKEN_OR, TOKEN_AND,
    TOKEN_WHILE, TOKEN_FOR,
    TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_RETURN,

    TOKEN_ERROR,

    TOKEN_EOF
} token_kind_t;

bool is_comparing_token(token_kind_t k);
bool is_computing_token(token_kind_t k);
bool is_logical_token(token_kind_t k);

const char *token_kind_to_str(token_kind_t k);

typedef struct {
    token_kind_t kind;
    span_t span;

    union {
        uint8_t char_val;
        uint64_t num_val;
        double float_val;
    } attr;
} token_t;

#define AVEC_TYPE token_t
#define AVEC_NAME token
#include "dsa/arena_vec.h"

#endif // TOKEN_H
