#include "parser/expr.h"

#include "common.h"
#include "mem/arena.h"
#include "parser/parser.h"
#include "scanner/token.h"
#include "str.h"

static expr_t *expr_assignment();
static expr_t *expr_or();
static expr_t *expr_and();
static expr_t *expr_equality();
static expr_t *expr_comparison();
static expr_t *expr_term();
static expr_t *expr_factor();
static expr_t *expr_unary();
static expr_t *expr_postfix();
static expr_t *finish_call(expr_t *callee);
static expr_t *expr_primary();

expr_t *expression() { return expr_assignment(); }

static expr_t *expr_assignment() {
    expr_t *left = expr_or();
    if (!left) return nullptr;
    if (!matches(TOKEN_EQUAL)) return left;

    token_t op = advance(); // Consume '='

    if (left->kind != EXPR_VAR) {
        report_err(left->span, "Invalid assignment target");
        return nullptr;
    }

    expr_t *right = expr_assignment();
    if (!right) return nullptr;

    expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
    *expr = (expr_t){
        .kind = EXPR_BINARY,
        .span = span_join(left->span, right->span),
        .binary = {.left = left, .op = op, .right = right}
    };
    return expr;
}

static expr_t *expr_or() {
    expr_t *left = expr_and();
    if (!left) return nullptr;

    while (matches(TOKEN_OR)) {
        const token_t op = advance();
        expr_t *right = expr_and();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_and() {
    expr_t *left = expr_equality();
    if (!left) return nullptr;

    while (matches(TOKEN_AND)) {
        const token_t op = advance();
        expr_t *right = expr_equality();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_equality() {
    expr_t *left = expr_comparison();
    if (!left) return nullptr;

    while (matches(TOKEN_BANG_EQUAL) || matches(TOKEN_EQUAL_EQUAL)) {
        const token_t op = advance();
        expr_t *right = expr_comparison();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_comparison() {
    expr_t *left = expr_term();
    if (!left) return nullptr;

    while (is_comparison_token(peek())) {
        const token_t op = advance();
        expr_t *right = expr_term();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_term() {
    expr_t *left = expr_factor();
    if (!left) return nullptr;

    while (matches(TOKEN_PLUS) || matches(TOKEN_MINUS)) {
        const token_t op = advance();
        expr_t *right = expr_factor();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_factor() {
    expr_t *left = expr_unary();
    if (!left) return nullptr;

    while (matches(TOKEN_STAR) || matches(TOKEN_SLASH) || matches(TOKEN_PERCENT)) {
        const token_t op = advance();
        expr_t *right = expr_unary();
        if (!right) return nullptr;

        expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
        *expr = (expr_t){
            .kind = EXPR_BINARY,
            .span = span_join(left->span, right->span),
            .binary = {.left = left, .op = op, .right = right}
        };
        left = expr;
    }

    return left;
}

static expr_t *expr_unary() {
    if (!is_unary_token(peek())) {
        return expr_postfix();
    }

    const token_t op = advance();
    expr_t *right = expr_unary();
    if (!right) return nullptr;

    expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
    *expr = (expr_t){
        .kind = EXPR_UNARY,
        .span = span_join(op.span, right->span),
        .unary = {.op = op, .right = right}
    };
    return expr;
}

static expr_t *expr_postfix() {
    expr_t *left = expr_primary();
    if (!left) return nullptr;

    while (matches(TOKEN_LEFT_PAREN)) {
        advance(); // Consume '('
        left = finish_call(left);
        if (!left) return nullptr;
    }
    return left;
}

static expr_t *finish_call(expr_t *callee) {
    if (callee->kind != EXPR_VAR) {
        report_err(callee->span, "expression is not callable");
        return nullptr;
    }

    expr_vec_t args = {};

    if (!matches(TOKEN_RIGHT_PAREN)) {
        for (;;) {
            expr_t *arg = expression();
            if (!arg) {
                expr_vec_destroy(&args);
                return nullptr;
            }
            expr_vec_push(&args, arg);

            if (!matches(TOKEN_COMMA)) break;
            advance(); // Consume ','
        }
    }

    const token_t right_paren = expect(TOKEN_RIGHT_PAREN);
    if (right_paren.kind == TOKEN_ERROR) {
        expr_vec_destroy(&args);
        return nullptr;
    }

    expr_vec_move(&args, parser.arena);

    expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
    *expr = (expr_t){
        .kind = EXPR_CALL,
        .span = span_join(callee->span, right_paren.span),
        .call = {.callee = callee->var, .args = args}
    };
    return expr;
}

static expr_t *expr_primary() {
    if (!has_more()) {
        report_err(peek().span, "Expected expression, found EOF");
        return nullptr;
    }

    const token_t next = advance();

    switch (next.kind) {
        case TOKEN_IDENTIFIER: {
            expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
            *expr = (expr_t){.kind = EXPR_VAR, .span = next.span, .var = next};
            return expr;
        }

        case TOKEN_INTEGER:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_CHAR:
        case TOKEN_FALSE:
        case TOKEN_TRUE: {
            expr_t *expr = arena_alloc(parser.arena, sizeof *expr);
            *expr = (expr_t){.kind = EXPR_PRIMARY, .span = next.span, .primary = next};
            return expr;
        }

        case TOKEN_LEFT_PAREN: {
            expr_t *inner = expression();
            if (!inner) return nullptr;

            const token_t right_paren = expect(TOKEN_RIGHT_PAREN);
            if (right_paren.kind == TOKEN_ERROR) return nullptr;

            // Parentheses don't need a node, they only widen the span
            inner->span = span_join(next.span, right_paren.span);
            return inner;
        }

        default:
            report_err(
                next.span,
                "Expected expression, found '" STR_FMT "' instead",
                STR_ARG(next.lexeme));
            return nullptr;
    }
}

type_t *expr_type() {
    if (!has_more()) {
        report_err(peek().span, "Expected type, found EOF");
        return nullptr;
    }

    const token_t next = advance();
    if (next.kind != TOKEN_IDENTIFIER) {
        report_err(next.span, "Expected type, found '" STR_FMT "'", STR_ARG(next.lexeme));
        return nullptr;
    }

    // Only primitive types exist for now, so they can be resolved while parsing.
    // User defined types will need to be resolved by the checker instead.
    for (size_t i = 0; i < prims_count; i++) {
        if (str_eq(prims[i]->lexeme, next.lexeme)) {
            return prims[i];
        }
    }

    report_err(next.span, "Unknown type '" STR_FMT "'", STR_ARG(next.lexeme));
    return nullptr;
}
