#include "parser/stmt.h"

#include "mem/arena.h"
#include "parser/expr.h"
#include "parser/parser.h"
#include "mem/pvec.h"
#include "scanner/token.h"

#define MAX_PARAMS 255

static stmt_t *stmt_scoped();
static stmt_t *stmt_if();
static stmt_t *stmt_while();
static stmt_t *stmt_for();
static stmt_t *stmt_var_decl();
static stmt_t *stmt_fun_decl();
static stmt_t *stmt_return();
static stmt_t *stmt_expr();
static stmt_t *stmt_block();
static stmt_t *stmt_break();
static stmt_t *stmt_continue();

stmt_t *global_statement() {
    const token_t next = peek();

    switch (next.kind) {
        // Global variable declarations are a bit tricky, will have to spend
        // a little bit of time designing the behaviour
        // case TOKEN_LET: return stmt_var_decl();
        case TOKEN_FUN: return stmt_fun_decl();
        default: {
            advance();
            report_err(next.span, "expected a function declaration");
            return nullptr;
        }
    }
}

static stmt_t *stmt_scoped() {
    const token_t next = peek();

    switch (next.kind) {
        case TOKEN_IF: return stmt_if();
        case TOKEN_LET: return stmt_var_decl();
        case TOKEN_FOR: return stmt_for();
        case TOKEN_WHILE: return stmt_while();
        case TOKEN_FUN: return stmt_fun_decl();
        case TOKEN_RETURN: return stmt_return();
        case TOKEN_LEFT_BRACE: return stmt_block();
        case TOKEN_BREAK: return stmt_break();
        case TOKEN_CONTINUE: return stmt_continue();
        default: return stmt_expr();
    }
}

static stmt_t *stmt_if() {
    const token_t keyword = expect(TOKEN_IF);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *condition = expression();
    if (!condition) return nullptr;

    stmt_t *then_body = stmt_block();
    if (!then_body) return nullptr;

    stmt_t *else_body = nullptr;
    if (matches(TOKEN_ELSE)) {
        advance(); // Consume else

        // An else if is just an else whose body is another if statement
        else_body = matches(TOKEN_IF) ? stmt_if() : stmt_block();
        if (!else_body) return nullptr;
    }

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_IF,
        .span = span_join(keyword.span, parser.previous.span),
        .if_ = {
            .then_condition = condition,
            .then_body = then_body,
            .else_body = else_body
        }
    };
    return stmt;
}

static stmt_t *stmt_while() {
    const token_t keyword = expect(TOKEN_WHILE);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *condition = expression();
    if (!condition) return nullptr;

    stmt_t *body = stmt_block();
    if (!body) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_WHILE,
        .span = span_join(keyword.span, parser.previous.span),
        .while_ = {
            .condition = condition,
            .body = body
        }
    };
    return stmt;
}

static stmt_t *stmt_for() {
    const token_t keyword = expect(TOKEN_FOR);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    stmt_t *initializer = nullptr;
    if (matches(TOKEN_SEMICOLON)) {
        advance(); // Empty initializer
    } else {
        // Both statements consume their trailing ';'
        initializer = matches(TOKEN_LET) ? stmt_var_decl() : stmt_expr();
        if (!initializer) return nullptr;
    }

    expr_t *condition = nullptr;
    if (!matches(TOKEN_SEMICOLON)) {
        condition = expression();
        if (!condition) return nullptr;
    }

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    expr_t *increment = nullptr;
    if (!matches(TOKEN_LEFT_BRACE)) {
        increment = expression();
        if (!increment) return nullptr;
    }

    stmt_t *body = stmt_block();
    if (!body) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_FOR,
        .span = span_join(keyword.span, parser.previous.span),
        .for_ = {
            .initializer = initializer,
            .condition = condition,
            .increment = increment,
            .body = body
        }
    };
    return stmt;
}

static stmt_t *stmt_fun_decl() {
    const token_t keyword = expect(TOKEN_FUN);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    const token_t identifier = expect(TOKEN_IDENTIFIER);
    if (identifier.kind == TOKEN_ERROR) return nullptr;

    if (expect(TOKEN_LEFT_PAREN).kind == TOKEN_ERROR) return nullptr;

    fun_param_t *params[MAX_PARAMS];
    size_t param_count = 0;

    if (!matches(TOKEN_RIGHT_PAREN)) {
        for (;;) {
            const token_t param_name = expect(TOKEN_IDENTIFIER);
            if (param_name.kind == TOKEN_ERROR) return nullptr;

            if (expect(TOKEN_COLON).kind == TOKEN_ERROR) return nullptr;

            type_t *param_type = expr_type();
            if (param_type == nullptr) return nullptr;

            if (param_count == MAX_PARAMS) {
                report_err(param_name.span, "cannot have more than %d parameters", MAX_PARAMS);
            } else {
                fun_param_t *param = arena_alloc(parser.arena, sizeof *param);
                *param = (fun_param_t) {
                    .identifier = param_name,
                    .type = param_type
                };
                params[param_count++] = param;
            }

            if (!matches(TOKEN_COMMA)) break;
            advance();
        }
    }

    if (expect(TOKEN_RIGHT_PAREN).kind == TOKEN_ERROR) return nullptr;

    type_t *return_type = &ty_void;
    if (matches(TOKEN_ARROW)) {
        advance();
        return_type = expr_type();
        if (!return_type) return nullptr;
    }

    stmt_t *body = stmt_block();
    if (!body) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_FUN_DECL,
        .span = span_join(keyword.span, parser.previous.span),
        .fun = {
            .identifier = identifier,
            .params = arena_copy(parser.arena, params, param_count * sizeof(fun_param_t*)),
            .param_count = param_count,
            .return_type = return_type,
            .body = body
        }
    };
    return stmt;
}

static stmt_t *stmt_var_decl() {
    const token_t keyword = expect(TOKEN_LET);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    const token_t identifier = expect(TOKEN_IDENTIFIER);
    if (identifier.kind == TOKEN_ERROR) return nullptr;

    type_t *decl_type = nullptr;
    if (matches(TOKEN_COLON)) {
        advance();
        if ((decl_type = expr_type()) == nullptr) return nullptr;
    }

    if (expect(TOKEN_EQUAL).kind == TOKEN_ERROR) return nullptr;

    expr_t *value = expression();
    if (!value) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_VAR_DECL,
        .span = span_join(keyword.span, parser.previous.span),
        .var = {
            .identifier = identifier,
            .type = decl_type,
            .value = value
        }
    };
    return stmt;
}

static stmt_t *stmt_return() {
    const token_t keyword = expect(TOKEN_RETURN);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *value = nullptr;
    if (!matches(TOKEN_SEMICOLON)) {
        value = expression();
        if (!value) return nullptr;
    }

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_RETURN,
        .span = span_join(keyword.span, parser.previous.span),
        .return_ = {
            .value = value
        }
    };
    return stmt;
}

static stmt_t *stmt_block() {
    const token_t left_brace = expect(TOKEN_LEFT_BRACE);
    if (left_brace.kind == TOKEN_ERROR) return nullptr;

    pvec_t *stmts = pvec_init();

    while (has_more() && !matches(TOKEN_RIGHT_BRACE)) {
        stmt_t *inner = stmt_scoped();
        if (!inner) {
            pvec_destroy(stmts);
            return nullptr;
        }
        pvec_push(stmts, inner);
    }

    if (expect(TOKEN_RIGHT_BRACE).kind == TOKEN_ERROR) {
        pvec_destroy(stmts);
        return nullptr;
    }

    const size_t stmt_count = stmts->size;
    stmt_t **stmt_items = arena_copy(parser.arena, stmts->items, stmt_count * sizeof(stmt_t*));
    pvec_destroy(stmts);

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_BLOCK,
        .span = span_join(left_brace.span, parser.previous.span),
        .block = {
            .stmts = stmt_items,
            .stmt_count = stmt_count
        }
    };
    return stmt;
}

static stmt_t *stmt_break() {
    const token_t keyword = expect(TOKEN_BREAK);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_BREAK,
        .span = span_join(keyword.span, parser.previous.span)
    };
    return stmt;
}

static stmt_t *stmt_continue() {
    const token_t keyword = expect(TOKEN_CONTINUE);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_CONTINUE,
        .span = span_join(keyword.span, parser.previous.span)
    };
    return stmt;
}

static stmt_t *stmt_expr() {
    expr_t *expr = expression();
    if (!expr) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = arena_alloc(parser.arena, sizeof *stmt);
    *stmt = (stmt_t) {
        .kind = STMT_EXPR,
        .span = span_join(expr->span, parser.previous.span),
        .expr = expr
    };
    return stmt;
}
