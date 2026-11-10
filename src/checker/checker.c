#include "checker/checker.h"
#include "checker/scope.h"
#include "common.h"
#include "parser/expr.h"
#include "parser/stmt.h"
#include "scanner/token.h"
#include "str.h"
#include "types.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

checker_t checker;

static void checker_init();

static bool check_stmt(stmt_t *stmt);
static type_t *check_expr(expr_t *expr);

bool check(stmt_t **stmts, size_t stmt_count) {
    checker_init();

    // Initialize functions first to call regardless of declaration order
    for(size_t i = 0; i < stmt_count; i++) {
        stmt_t *stmt = stmts[i];
        if(stmt->kind != STMT_FUN_DECL) continue;

        declare_fun(stmt->fun.identifier, stmt->fun.params, stmt->fun.param_count, stmt->fun.return_type);
    }

    for(size_t i = 0; i < stmt_count; i++) {
        stmt_t *stmt = stmts[i];
        bool res = check_stmt(stmt);
        if(!res) {
            checker.has_error = true;
        }
    }

    scope_destroy(checker.scope);
    checker.scope = nullptr;

    return !checker.has_error;
}

static void checker_init() {
    if(checker.scope) {
        scope_destroy(checker.scope);
    }
    scope_t *global = malloc(sizeof *global);
    *global = (scope_t) {
        .kind = SCOPE_GLOBAL
    };
    checker.scope = global;
    bool had_error = false;
}

static bool check_stmt(stmt_t *stmt) {
    switch(stmt->kind) {
        case STMT_FUN_DECL: {
            push_fun_scope(stmt->fun.return_type);
            for(size_t i = 0; i < stmt->fun.param_count; i++) {
                declare_var(stmt->fun.params[i]->identifier, stmt->fun.params[i]->type);
            }

            bool res = check_stmt(stmt->fun.body);

            pop_scope(SCOPE_FUN);

            return res;
        }
        case STMT_VAR_DECL: {
            token_t identifier = stmt->var.identifier;

            type_t *inferred_type = check_expr(stmt->var.value);
            if(!inferred_type) return false;

            if(stmt->var.type && stmt->var.type != inferred_type) {
                report_err(identifier.span, "declared type differs from assigned value");
                return false;
            }
            if(!stmt->var.type) {
                stmt->var.type = inferred_type;
            }

            return declare_var(identifier, inferred_type);
        }
        case STMT_BLOCK: {
            bool had_error = false;
            for(size_t i = 0; i < stmt->block.stmt_count; i++) {
                bool res = check_stmt(stmt->block.stmts[i]);
                had_error = had_error || res;
            }
            return had_error;
        }
        case STMT_RETURN: {
            scope_t *fun_scope = search_scope(SCOPE_FUN);
            if(!fun_scope) {
                report_err(stmt->span, "returned outside of a function");
                return false;
            }

            type_t *return_type = check_expr(stmt->return_.value);
            if(!return_type) return false;

            if(return_type != fun_scope->return_type) {
                report_err(stmt->span,
                    "mismatched return type (expected " STR_FMT " received " STR_FMT " instead",
                    STR_ARG(fun_scope->return_type->lexeme), STR_ARG(return_type->lexeme));
                return false;
            }
            return true;
        }
        default: return false;
    }
}

static type_t *check_binary(expr_t *expr);

static type_t *check_expr(expr_t *expr) {
    switch (expr->kind) {
        case EXPR_BINARY: return check_binary(expr);
        case EXPR_UNARY: {
            token_t op = expr->unary.op;

            type_t *val_type = check_expr(expr->unary.right);
            if(!val_type) return nullptr;

            switch (op.kind) {
                case TOKEN_BANG: {
                    if(val_type != &ty_bool) {
                        report_err(expr->span, "Bang (!) operator can only be applied to boolean values");
                        return nullptr;
                    }
                    break;
                }
                case TOKEN_MINUS: {
                    if(!is_negatable(val_type)) {
                        report_err(expr->span, "Minus (-) operator can only be applied to integer and float values");
                        return nullptr;
                    }
                    break;
                }
                default: {
                    report_err(expr->span, "Invalid unary operator");
                    return nullptr;
                }
            }

            return val_type;
        }
        case EXPR_CALL: {
            token_t callee = expr->call.callee;
            obj_t *fun_obj = get_obj(callee.lexeme);
            if(!fun_obj) {
                report_err(callee.span, "Called function is not defined");
                return nullptr;
            }
            if(fun_obj->fun.param_count != expr->call.arg_count) {
                report_err(callee.span,
                    "Wrong number of arguments (expected %zu, received %zu)",
                    fun_obj->fun.param_count, expr->call.arg_count);
                return nullptr;
            }

            bool had_err = false;
            for(size_t i = 0; i < expr->call.arg_count; i++) {
                type_t *arg_type = check_expr(expr->call.args[i]);
                type_t *param_type = fun_obj->fun.params[i]->type;
                if(!arg_type){
                    had_err = true;
                    continue;
                }
                if(arg_type != param_type) {
                    report_err(expr->call.args[i]->span,
                        "Mismatched types (expected " STR_FMT ", received " STR_FMT " instead",
                        STR_ARG(param_type->lexeme), STR_ARG(arg_type->lexeme));
                    had_err = true;
                }
            }
            if(had_err) return nullptr;

            return fun_obj->fun.return_type;
        }
        case EXPR_VAR: {
            obj_t *val_obj = get_obj(expr->var.lexeme);
            if(!val_obj) {
                report_err(expr->span, "Undeclared variable");
                return nullptr;
            }
            return val_obj->var.type;
        }
        case EXPR_PRIMARY: {
            switch(expr->primary.kind) {
                case TOKEN_INTEGER: return &ty_int;
                case TOKEN_FLOAT: return &ty_float;
                case TOKEN_TRUE:
                case TOKEN_FALSE: return &ty_bool;
                case TOKEN_CHAR: return &ty_char;
                case TOKEN_STRING: return &ty_string;
                default: break;
            }

            report_err(expr->span, "Unrecognized primary expression");
            return nullptr;
        }
        default: {
            report_err(expr->span, "Unrecognized expression");
            return nullptr;
        }
    }
}

static type_t *check_binary(expr_t *expr) {
    type_t *left_type = check_expr(expr->binary.left);
    type_t *right_type = check_expr(expr->binary.right);
    if(!left_type || !right_type) {
        return nullptr;
    }
    if(left_type != right_type) {
        report_err(expr->span,
            "Mismatched operands types " STR_FMT " and " STR_FMT,
            STR_ARG(left_type->lexeme), STR_ARG(right_type->lexeme));
        return nullptr;
    }

    switch (expr->binary.op.kind) {
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
        case TOKEN_MINUS: {
            if(!is_computable(left_type)) {
                report_err(expr->span,
                    "Type " STR_FMT "cannot be computed",
                    STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_PLUS: {
            if(!is_computable(left_type) && left_type != &ty_string) {
                report_err(expr->span,
                    "type " STR_FMT "cannot be added",
                    STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL: {
            if(!is_comparable(left_type)) {
                report_err(expr->span,
                    "Type " STR_FMT "cannot be compared",
                    STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL: {
            if(!is_comparable(left_type) && left_type != &ty_string && left_type != &ty_char) {
                report_err(expr->span,
                    "Type " STR_FMT "cannot be compared",
                    STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        default: {
            report_err(expr->span, "Unrecognized binary expression");
            return nullptr;
        }
    }
}
