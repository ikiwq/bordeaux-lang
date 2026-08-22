#include "scanner/token.h"

const char *token_kind_to_str(const token_kind_t k) {
    switch (k) {
        case TOKEN_LEFT_PAREN:    return "(";
        case TOKEN_RIGHT_PAREN:   return ")";
        case TOKEN_LEFT_BRACKET:  return "[";
        case TOKEN_RIGHT_BRACKET: return "]";
        case TOKEN_LEFT_BRACE:    return "{";
        case TOKEN_RIGHT_BRACE:   return "}";
        case TOKEN_COMMA:         return ",";
        case TOKEN_DOT:           return ".";
        case TOKEN_MINUS:         return "-";
        case TOKEN_PLUS:          return "+";
        case TOKEN_COLON:         return ":";
        case TOKEN_SEMICOLON:     return ";";
        case TOKEN_SLASH:         return "/";
        case TOKEN_STAR:          return "*";
        case TOKEN_BANG:          return "!";
        case TOKEN_AMPERSAND:     return "&";
        case TOKEN_PIPE:          return "|";
        case TOKEN_BANG_EQUAL:    return "!=";
        case TOKEN_EQUAL:         return "=";
        case TOKEN_EQUAL_EQUAL:   return "==";
        case TOKEN_GREATER:       return ">";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LESS:          return "<";
        case TOKEN_LESS_EQUAL:    return "<=";
        case TOKEN_MODULO:        return "%";
        case TOKEN_ARROW:         return "->";

        case TOKEN_AND:           return "&&";
        case TOKEN_OR:            return "||";

        case TOKEN_IDENTIFIER:    return "identifier";
        case TOKEN_STRING:        return "string";
        case TOKEN_INTEGER:       return "integer";
        case TOKEN_FLOAT:         return "float";
        case TOKEN_CHAR:          return "character";

        case TOKEN_TRUE:          return "true";
        case TOKEN_FALSE:         return "false";

        case TOKEN_STRUCT:        return "struct";
        case TOKEN_LET:           return "let";
        case TOKEN_FUN:           return "fun";
        case TOKEN_IF:            return "if";
        case TOKEN_ELSE:          return "else";
        case TOKEN_WHILE:         return "while";
        case TOKEN_FOR:           return "for";
        case TOKEN_BREAK:         return "break";
        case TOKEN_CONTINUE:      return "continue";
        case TOKEN_RETURN:        return "return";

        case TOKEN_EOF:           return "EOF";
        default:                  return "<unknown>";
    }
}

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
