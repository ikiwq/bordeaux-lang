#include "scanner/token.h"

static token_kind_t comparing_tokens[] = {
    TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL
};

bool is_comparing_token(const token_kind_t k) {
    for(size_t i = 0; i < ARRAY_SIZE(comparing_tokens); i++) {
        if(k == comparing_tokens[i]) return true;
    }
    return false;
}

static token_kind_t computing_tokens[] = {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
};
bool is_computing_token(const token_kind_t k) {
    for (size_t i = 0; i < ARRAY_SIZE(computing_tokens); i++) {
        if (k == computing_tokens[i])  return true;
    }
    return false;
}

bool is_logical_token(const token_kind_t k) {
    return k == TOKEN_AND || k == TOKEN_OR;
}

static token_kind_t comparison_tokens[] = { TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL };

bool is_comparison_token(const token_t token) {
    for (size_t i = 0; i < ARRAY_SIZE(comparison_tokens); i++) {
        if (token.kind == comparison_tokens[i]) return true;
    }
    return false;
}

static token_kind_t unary_tokens[] = { TOKEN_MINUS, TOKEN_BANG };
bool is_unary_token(const token_t token) {
    for (size_t i = 0; i < ARRAY_SIZE(unary_tokens); i++) {
        if (token.kind == unary_tokens[i]) return true;
    }
    return false;
}
