#include "parser/parser.h"

#include <stdarg.h>
#include <stdio.h>

#include "mem/mem.h"
#include <string.h>

#include "dsa/vector.h"

VEC_DEFINE_H(stmt_t*, pstmt)
VEC_DEFINE_H(fun_parameter_t, fnparam)
VEC_DEFINE_H(expr_t*, pexpr)

static parser_t parser;

static void init_parser(token_t*, size_t);

static void panic_mode();

static stmt_t *global_statement();
static stmt_t *scoped_statement();
static stmt_t *if_statement();
static stmt_t *while_statement();
static stmt_t *for_statement();
static stmt_t *var_declaration_statement();
static stmt_t *fun_declaration_statement();
static stmt_t *return_statement();
static stmt_t *expression_statement();
static stmt_t *block_statement();
static stmt_t *break_statement();
static stmt_t *continue_statement();
static stmt_t *stmt_new();

static expr_t *expression();
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
static expr_t *array_literal(token_t left_bracket);
static expr_t *expr_type();
static expr_t *expr_make();

static bool has_more();
static token_t peek();
static token_t advance();
static token_t last();

static token_t expect(token_kind_t expected);
static bool matches(token_kind_t expected);

parser_result_t parse(token_t *tokens, const size_t token_count) {
    init_parser(tokens, token_count);
    pstmt_vec_t statement_vec = pstmt_vec_init();

    while (has_more() && peek().kind != TOKEN_EOF) {
        stmt_t *stmt = global_statement();
        if (!stmt) {
            panic_mode();
            continue;
        }
        pstmt_vec_push(&statement_vec, stmt);
    }

    const size_t err_count = parser.err_vec.size;
    fe_err_t *errs =
        arena_copy(&parser.arena, parser.err_vec.data, err_count * sizeof *errs);
    err_vec_destroy(&parser.err_vec);

    const size_t stmt_count = statement_vec.size;
    stmt_t **stmts =
        arena_copy(&parser.arena, statement_vec.data, stmt_count * sizeof *stmts);
    pstmt_vec_destroy(&statement_vec);

    const parser_result_t result = {
        .errs = errs,
        .err_count = err_count,
        .stmts = stmts,
        .stmt_count= stmt_count,
        .arena = parser.arena
    };

    // Ensure that the arena is no longer used in this scope.
    // The caller is still the owner of the arena
    parser.arena = (arena_t){0};

    return result;
}

static void init_parser(token_t *tokens, const size_t token_count) {
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.current = 0;
    parser.err_vec = err_vec_init();
    parser.arena = arena_make(1 << 28u); // 256 MB
}

static token_kind_t sync_tokens[] = {
    TOKEN_LET, TOKEN_FUN, TOKEN_STRUCT,
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

static void make_err(const span_t span, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    const int len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (len < 0) return;

    char *buf = arena_alloc(&parser.arena, (size_t)len + 1);
    if (!buf) return;

    va_start(args, fmt);
    vsnprintf(buf, (size_t)len + 1, fmt, args);
    va_end(args);

    err_vec_push(&parser.err_vec, (fe_err_t) {
        .err = buf,
        .span = span
    });
}

static stmt_t *global_statement() {
    const token_t next = peek();

    switch (next.kind) {
        case TOKEN_LET: return var_declaration_statement();
        case TOKEN_FUN: return fun_declaration_statement();
        default: {
            advance();
            make_err(next.span, "expected a declaration");
            return nullptr;
        }
    }
}

static stmt_t *scoped_statement() {
    const token_t next = peek();

    switch (next.kind) {
        case TOKEN_IF: return if_statement();
        case TOKEN_LET: return var_declaration_statement();
        case TOKEN_FOR: return for_statement();
        case TOKEN_WHILE: return while_statement();
        case TOKEN_FUN: return fun_declaration_statement();
        case TOKEN_RETURN: return return_statement();
        case TOKEN_LEFT_BRACE: return block_statement();
        case TOKEN_BREAK: return break_statement();
        case TOKEN_CONTINUE: return continue_statement();
        default: return expression_statement();
    }
}

static stmt_t *if_statement() {
    const token_t keyword = expect(TOKEN_IF);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *condition = expression();
    if (!condition) return nullptr;

    stmt_t *then_branch = block_statement();
    if (!then_branch) return nullptr;

    stmt_t *else_branch = nullptr;
    if (matches(TOKEN_ELSE)) {
        advance();

        else_branch = block_statement();
        if (!else_branch) return nullptr;
    }

    const size_t end = else_branch != nullptr ? else_branch->span.end : then_branch->span.end;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_IF,
        .span = (span_t) {
            .src = keyword.span.src,
            .length = end - keyword.span.start,
            .start = keyword.span.start,
            .end = end
        },
        .as._if = {
            .condition = condition,
            .then_branch = then_branch,
            .else_branch = else_branch,
        }
    };
    return stmt;
}

static stmt_t *while_statement() {
    const token_t keyword = expect(TOKEN_WHILE);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *condition = expression();
    if (!condition) return nullptr;

    stmt_t *body = block_statement();
    if (!body) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_WHILE,
        .span = {
            .src = keyword.span.src,
            .length = (body->span.end - keyword.span.start),
            .start = keyword.span.start,
            .end = keyword.span.end
        },
        .as._while= {
            .condition = condition,
            .body = body
        }
    };
    return stmt;
}

static stmt_t *for_statement() {
    const token_t keyword = expect(TOKEN_FOR);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    stmt_t *init = nullptr;
    if (matches(TOKEN_SEMICOLON)) {
        advance();
    } else if (matches(TOKEN_LET)) {
        init = var_declaration_statement();
        if (!init) return nullptr;
    } else {
        init = expression_statement();
        if (!init) return nullptr;
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

    stmt_t *body = block_statement();
    if (!body) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_FOR,
        .span = {
            .src = keyword.span.src,
            .length = (body->span.end - keyword.span.start),
            .start = keyword.span.start,
            .end = body->span.end
        },
        .as._for = {
            .init = init,
            .condition = condition,
            .increment = increment,
            .body = body
        }
    };
    return stmt;
}

static stmt_t *fun_declaration_statement() {
    const token_t keyword = expect(TOKEN_FUN);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    const token_t identifier = expect(TOKEN_IDENTIFIER);
    if (identifier.kind == TOKEN_ERROR) return nullptr;

    if (expect(TOKEN_LEFT_PAREN).kind == TOKEN_ERROR) return nullptr;

    fnparam_vec_t param_vec = fnparam_vec_init();
    if (peek().kind != TOKEN_RIGHT_PAREN) {
        for (;;) {
            const token_t param_name = expect(TOKEN_IDENTIFIER);
            if (param_name.kind == TOKEN_ERROR) goto fail;

            if (expect(TOKEN_COLON).kind == TOKEN_ERROR) goto fail;
            expr_t *param_type = expr_type();

            if (param_type == nullptr) goto fail;

            const fun_parameter_t parameter = (fun_parameter_t) {
                .name = param_name,
                .type = param_type,
            };
            fnparam_vec_push(&param_vec, parameter);
            if (!matches(TOKEN_COMMA)) break;
            advance();
        }
    }

    if (expect(TOKEN_RIGHT_PAREN).kind == TOKEN_ERROR) goto fail;

    expr_t *return_type = nullptr;
    if (matches(TOKEN_COLON)) {
        advance();
        return_type = expr_type();
        if (!return_type) goto fail;
    }

    stmt_t *body = block_statement();
    if (!body) goto fail;

    const size_t params_count = param_vec.size;
    fun_parameter_t *params = nullptr;

    // Move params from random heap to arena, so we can free it along the other nodes
    if (params_count > 0) {
        const size_t data_size = sizeof *params * param_vec.size;
        params = arena_alloc(&parser.arena, data_size);
        memcpy(params, param_vec.data, data_size);
    }
    fnparam_vec_destroy(&param_vec);

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_FUN_DECLARATION,
        .span = {
            .src = keyword.span.src,
            .length = (body->span.end - keyword.span.start),
            .start = keyword.span.start,
            .end = body->span.end
        },
        .as.fun_decl = {
            .identifier = identifier,
            .params_count = params_count,
            .params = params,
            .return_type = return_type,
            .body = body
        }
    };
    return stmt;

fail:
    fnparam_vec_destroy(&param_vec);
    return nullptr;
}

static stmt_t *var_declaration_statement() {
    const token_t keyword = expect(TOKEN_LET);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    const token_t identifier = expect(TOKEN_IDENTIFIER);
    if (identifier.kind == TOKEN_ERROR) return nullptr;

    expr_t *type = nullptr;
    if (matches(TOKEN_COLON)) {
        advance();
        if ((type = expr_type()) == nullptr) return nullptr;
    }

    if (expect(TOKEN_EQUAL).kind == TOKEN_ERROR) return nullptr;

    expr_t *initializer = expression();
    if (!initializer) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_VAR_DECLARATION,
        .as.var_decl = {
            .identifier = identifier,
            .type = type,
            .initializer = initializer,
        }
    };

    return stmt;
}

static stmt_t *return_statement() {
    const token_t keyword = expect(TOKEN_RETURN);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    expr_t *value = nullptr;
    if (!matches(TOKEN_SEMICOLON)) {
        value = expression();
        if (!value) return nullptr;
    }
    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_RETURN,
        .span = {
            .src = keyword.span.src,
            .length = value ? (value->span.end - keyword.span.start) : keyword.span.length,
            .start = keyword.span.start,
            .end = value ? value->span.end : keyword.span.end,
        },
        .as._return = {
            .keyword = keyword,
            .value = value
        }
    };

    return stmt;
}

static stmt_t *block_statement() {
    const token_t left_brace = expect(TOKEN_LEFT_BRACE);
    if (left_brace.kind == TOKEN_ERROR) return nullptr;

    pstmt_vec_t stmt_vec = pstmt_vec_init();
    while (has_more() && peek().kind != TOKEN_RIGHT_BRACE) {
        stmt_t *stmt = scoped_statement();
        if (!stmt) goto fail;
        pstmt_vec_push(&stmt_vec, stmt);
    }

    const token_t right_brace = expect(TOKEN_RIGHT_BRACE);
    if (right_brace.kind == TOKEN_ERROR) goto fail;

    const size_t statements_count = stmt_vec.size;
    stmt_t **statements = arena_copy(&parser.arena, stmt_vec.data, statements_count * sizeof *statements);
    pstmt_vec_destroy(&stmt_vec);

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_BLOCK,
        .span = {
            .src = left_brace.span.src,
            .length = (right_brace.span.end - left_brace.span.start),
            .start = left_brace.span.start,
            .end = right_brace.span.end
        },
        .as.block = {
            .stmts = statements,
            .stmt_count = statements_count,
        }
    };
    return stmt;

fail:
    pstmt_vec_destroy(&stmt_vec);
    return nullptr;
}

static stmt_t *break_statement() {
    const token_t keyword = expect(TOKEN_BREAK);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_BREAK,
        .span = keyword.span,
    };
    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    return stmt;
}

static stmt_t *continue_statement() {
    const token_t keyword = expect(TOKEN_CONTINUE);
    if (keyword.kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_CONTINUE,
        .span = keyword.span,
    };
    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    return stmt;
}

static stmt_t *expression_statement() {
    expr_t *expr = expression();
    if (!expr) return nullptr;

    if (expect(TOKEN_SEMICOLON).kind == TOKEN_ERROR) return nullptr;

    stmt_t *stmt = stmt_new();
    *stmt = (stmt_t) {
        .kind = STMT_EXPR,
        .span = expr->span,
        .as.expr = expr
    };
    return stmt;
}

static stmt_t *stmt_new() {
    return arena_alloc(&parser.arena, sizeof(stmt_t));
}

static expr_t *expression() {
    return expr_assignment();
}

static expr_t *expr_assignment() {
    expr_t *left = expr_or();
    if (!left) return nullptr;
    if (!matches(TOKEN_EQUAL)) return left;

    const token_t equals = advance();

    if (!is_assignable(left)) {
        make_err(left->span, "Invalid assignment target");
        return nullptr;
    }

    expr_t *value = expr_assignment();
    if (!value) return nullptr;

    expr_t *new_left = expr_make();
    *new_left = (expr_t) {
        .kind = EXPR_BINARY,
        .span = {
            .src = left->span.src,
            .length = value->span.end - left->span.start,
            .start = left->span.start,
            .end = value->span.end,
        },
        .as.binary = {
            .left = left,
            .op = equals,
            .right = value
        }
    };

    return new_left;
}

static expr_t *expr_or() {
    expr_t *left = expr_and();
    if (!left) return nullptr;

    while (matches(TOKEN_OR)) {
        const token_t op = advance();
        expr_t *right = expr_and();
        if (!right) return nullptr;

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.end
            },
            .as.binary = {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
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

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.start,
            },
            .as.binary = {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
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

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.end
            },
            .as.binary = {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
    }

    return left;
}

static token_kind_t comparison_tokens[] = { TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL };

static bool is_comparison_token(const token_t token) {
    for (size_t i = 0; i < ARRAY_SIZE(comparison_tokens); i++) {
        if (token.kind == comparison_tokens[i]) return true;
    }
    return false;
}

static expr_t *expr_comparison() {
    expr_t *left = expr_term();
    if (!left) return nullptr;

    while (is_comparison_token(peek())) {
        const token_t op = advance();
        expr_t *right = expr_term();
        if (!right) return nullptr;

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.end
            },
            .as.binary = {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
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

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.end
            },
            .as.binary = {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
    }

    return left;
}

static expr_t *expr_factor() {
    expr_t *left = expr_unary();
    if (!left) return nullptr;

    while (matches(TOKEN_STAR) || matches(TOKEN_SLASH) || matches(TOKEN_MODULO)) {
        const token_t op = advance();
        expr_t *right = expr_unary();
        if (!right) return nullptr;

        expr_t *new_left = expr_make();
        *new_left = (expr_t) {
            .kind = EXPR_BINARY,
            .span = {
                .src = left->span.src,
                .length = right->span.end - left->span.start,
                .start = left->span.start,
                .end = right->span.end
            },
            .as.binary= {
                .left = left,
                .op = op,
                .right = right
            }
        };

        left = new_left;
    }

    return left;
}

static token_kind_t unary_tokens[] = { TOKEN_MINUS, TOKEN_BANG, TOKEN_AND, TOKEN_AMPERSAND, TOKEN_STAR };
bool is_unary_token(const token_t token) {
    for (size_t i = 0; i < ARRAY_SIZE(unary_tokens); i++) {
        if (token.kind == unary_tokens[i]) return true;
    }
    return false;
}

static expr_t *make_unary(const token_t op, expr_t *operand) {
    expr_t *node = expr_make();
    *node = (expr_t) {
        .kind = EXPR_UNARY,
        .span = {
            .src = op.span.src,
            .length = operand->span.end - op.span.start,
            .start = op.span.start,
            .end = operand->span.end
        },
        .as.unary = { .op = op, .right = operand }
    };
    return node;
}

static expr_t *expr_unary() {
    if (is_unary_token(peek())) {
        const token_t op = advance();
        expr_t *right = expr_unary();
        if (!right) return nullptr;

        if (op.kind == TOKEN_AND) {
            const token_t first_amp = (token_t) {
                .kind = TOKEN_AMPERSAND,
                .span = {
                    .src = op.span.src,
                    .length = 1,
                    .start = op.span.start,
                    .end = op.span.end + 1,
                }
            };
            const token_t second_amp = (token_t) {
                .kind = TOKEN_AMPERSAND,
                .span = {
                    .src = op.span.src + 1,
                    .length = 1,
                    .start = op.span.start + 1,
                    .end = op.span.end + 2
                }
            };
            return make_unary(first_amp, make_unary(second_amp, right));
        }

        return make_unary(op, right);
    }
    return expr_postfix();
}

static expr_t *expr_postfix() {
    expr_t *left = expr_primary();
    if (!left) return nullptr;

    for (;;) {
        if (matches(TOKEN_LEFT_PAREN)) {
            advance();
            left = finish_call(left);
            if (!left) return nullptr;
        }
        else if (matches(TOKEN_LEFT_BRACKET)) {
            advance();
            expr_t *index = expression();
            if (!index || expect(TOKEN_RIGHT_BRACKET).kind == TOKEN_ERROR) return nullptr;

            expr_t *node = expr_make();
            *node = (expr_t){
                .kind = EXPR_INDEX,
                .span = {
                    .src = left->span.src,
                    .length = index->span.end - left->span.start,
                    .start = left->span.start,
                    .end = index->span.end
                },
                .as.index = { .array = left, .index = index },
            };
            left = node;
        }
        else {
            break;
        }
    }
    return left;
}

static expr_t *finish_call(expr_t *callee) {
    pexpr_vec_t expr_vec = pexpr_vec_init();

    if (!matches(TOKEN_RIGHT_PAREN)) {
        for (;;) {
            expr_t *arg = expression();
            if (!arg) goto fail;
            pexpr_vec_push(&expr_vec, arg);

            if (!matches(TOKEN_COMMA)) break;
            advance();
        }
    }
    const token_t right_paren = expect(TOKEN_RIGHT_PAREN);
    if (right_paren.kind == TOKEN_ERROR) goto fail;

    const size_t args_count = expr_vec.size;
    expr_t **args = arena_copy(&parser.arena, expr_vec.data, sizeof(expr_t*) * args_count);
    pexpr_vec_destroy(&expr_vec);

    expr_t *expr = expr_make();
    *expr = (expr_t) {
        .kind = EXPR_CALL,
        .span = {
            .src = callee->span.src,
            .length = right_paren.span.end - callee->span.start,
            .start = callee->span.start,
            .end = right_paren.span.end
        },
        .as.call = {
            .callee = callee,
            .args = args,
            .args_count = args_count
        }
    };

    return expr;
fail:
    pexpr_vec_destroy(&expr_vec);
    return nullptr;
}

static expr_t *expr_primary() {
    if (!has_more()) {
        make_err(last().span, "Expected expression, found EOF");
        return nullptr;
    }

    const token_t next = advance();
    expr_t *expr = expr_make();

    switch (next.kind) {
        case TOKEN_IDENTIFIER:
            *expr = (expr_t){
                .kind = EXPR_IDENTIFIER,
                .span = next.span,
                .as.identifier = next
            };
            break;
        case TOKEN_INTEGER: {
            *expr = (expr_t){
                .kind = EXPR_LITERAL,
                .span = next.span,
                .as.literal = { .kind = LIT_INTEGER,._int = next.attr.num_val },
            };
            break;
        }

        case TOKEN_FLOAT: {
            *expr = (expr_t){
                .kind = EXPR_LITERAL,
                .span = next.span,
                .as.literal = { .kind = LIT_FLOAT, ._float = next.attr.float_val },
            };
            break;
        }

        case TOKEN_STRING:
            *expr = (expr_t){
                .kind = EXPR_LITERAL,
                .span = next.span,
                .as.literal = {
                    .kind = LIT_STRING,
                    .string = {
                        .start = next.span.src,
                        .length = next.span.length
                    }
                },
            };
            break;

        case TOKEN_FALSE:
        case TOKEN_TRUE:
            *expr = (expr_t){
                .kind = EXPR_LITERAL,
                .span = next.span,
                .as.literal = {
                    .kind = LIT_BOOL,
                    ._bool = next.kind == TOKEN_TRUE ? true : false
                },
            };
            break;


        case TOKEN_LEFT_PAREN: {
            expr_t *inner = expression();
            if (!inner) return nullptr;

            const token_t right_paren = expect(TOKEN_RIGHT_PAREN);
            if (right_paren.kind == TOKEN_ERROR) return nullptr;

            *expr = (expr_t) {
                .kind = EXPR_GROUP,
                .span = {
                    .src = next.span.src,
                    .length = right_paren.span.end - next.span.start,
                    .start = next.span.start,
                    .end = right_paren.span.end
                },
                .as.group = {
                    .left_paren = next,
                    .inner = inner,
                    .right_paren = right_paren
                }
            };
            break;
        }

        case TOKEN_LEFT_BRACKET:
            return array_literal(next);

        default:
            make_err(next.span, "Expected expression, found %.*s instead", next.span.length, next.span.src);
            return nullptr;
    }

    return expr;
}

static expr_t *array_literal(token_t left_bracket) {
    pexpr_vec_t elems = pexpr_vec_init();

    if (!matches(TOKEN_RIGHT_BRACKET)) {
        for (;;) {
            expr_t *el = expression();
            if (!el) goto fail;
            pexpr_vec_push(&elems, el);

            if (!matches(TOKEN_COMMA)) break;
            advance();
        }
    }

    const token_t right_bracket = expect(TOKEN_RIGHT_BRACKET);
    if (right_bracket.kind == TOKEN_ERROR) goto fail;

    const size_t count = elems.size;
    expr_t **items = arena_copy(&parser.arena, elems.data, sizeof *items * count);
    pexpr_vec_destroy(&elems);

    expr_t *expr = expr_make();
    *expr = (expr_t){
        .kind = EXPR_ARR_LITERAL,
        .span = {
            .src = left_bracket.span.src,
            .length = left_bracket.span.length,
            .start = left_bracket.span.start,
            .end = right_bracket.span.end
        },
        .as.array_literal = { .elements = items, .element_count = count },
    };
    return expr;

fail:
    pexpr_vec_destroy(&elems);
    return nullptr;
}

static expr_t *expr_make() {
    return arena_alloc(&parser.arena, sizeof(expr_t));
}

static expr_t *expr_type() {
    if (!has_more()) {
        make_err(last().span, "Expected type, found EOF");
        return nullptr;
    }

    const token_t next = advance();
    expr_t *type = expr_make();

    switch (next.kind) {
        case TOKEN_IDENTIFIER: {
            *type = (expr_t) {
                .kind = EXPR_TYPE_NAME,
                .span = next.span,
                .as.type_name = next
            };
            break;
        }

        case TOKEN_LEFT_BRACKET: {
            expr_t *len = nullptr;
            if (!matches(TOKEN_RIGHT_BRACKET)) {
                len = expression();
                if (!len) return nullptr;
            }
            const token_t right_bracket = expect(TOKEN_RIGHT_BRACKET);
            if (right_bracket.kind == TOKEN_ERROR) return nullptr;

            expr_t *inner = expr_type();
            if (!inner) return nullptr;

            const span_t span = {
                .src = next.span.src,
                .length = right_bracket.span.end - next.span.start,
                .start = next.span.start,
                .end = right_bracket.span.end
            };

            if (len == nullptr) {
                *type = (expr_t) {
                    .kind = EXPR_SLICE_TYPE,
                    .span = span,
                    .as.slice_type = {
                        .element = inner
                    }
                };
                break;
            }

            *type = (expr_t) {
                .kind = EXPR_ARR_TYPE,
                .span = span,
                .as.array_type = {
                    .element = inner,
                    .len = len
                }
            };
            break;
        }

        // Just treat && normally as a reference to a reference
        case TOKEN_AND: {
            expr_t *last = expr_type();
            if (!last) return nullptr;

            expr_t *inner = expr_make();
            *inner = (expr_t) {
                .kind = EXPR_REFERENCE_TYPE,
                .span = last->span,
                .as.reference_type = last
            };

            *type = (expr_t) {
                .kind = EXPR_REFERENCE_TYPE,
                .span = {
                    .src = next.span.src,
                    .length = last->span.end - next.span.start,
                    .start = next.span.start,
                    .end = last->span.end
                },
                .as.reference_type = inner
            };
            break;
        }

        case TOKEN_AMPERSAND: {
            expr_t *inner = expr_type();
            if (!inner) return nullptr;

            *type = (expr_t) {
                .kind = EXPR_REFERENCE_TYPE,
                .span = {
                    .src = next.span.src,
                    .length = inner->span.end - next.span.start,
                    .start = next.span.start,
                    .end = inner->span.end
                },
                .as.reference_type = inner
            };
            break;
        }

        case TOKEN_STAR: {
            expr_t *inner = expr_type();
            if (!inner) return nullptr;

            *type = (expr_t) {
                .kind = EXPR_POINTER_TYPE,
                .span = {
                    .src = next.span.src,
                    .length = inner->span.end - next.span.start,
                    .start = next.span.start,
                    .end = inner->span.end
                },
                .as.pointer_type = inner
            };
            break;
        }

        default: {
            make_err(next.span, "Expected type, found %.*s", next.span.length, next.span.src);
            return nullptr;
        }
    }

    return type;
}

static bool has_more() {
    return parser.current < parser.token_count;
}

static token_t advance() {
    return parser.tokens[parser.current++];
}

static token_t peek() {
    return parser.tokens[parser.current];
}

static token_t last() {
    return parser.tokens[parser.token_count - 1];
}

static token_t expect(const token_kind_t expected) {
    const token_t next = advance();
    if (next.kind != expected) {
        const char *t1 = token_kind_to_str(expected);
        const char *t2 = token_kind_to_str(next.kind);
        make_err(next.span, "Expected %s but found %s instead", t1, t2);
        return (token_t) {
            .kind = TOKEN_ERROR
        };
    }

    return next;
}

static bool matches(const token_kind_t expected) {
    return peek().kind == expected;
}
