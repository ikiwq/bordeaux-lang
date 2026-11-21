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

bool check(stmt_vec_t stmts) {
    checker_init();

    for (size_t i = 0; i < stmts.size; i++) {
        stmt_t *stmt = stmts.data[i];
        bool res = check_stmt(stmt);
        if (!res) {
            checker.has_error = true;
        }
    }

    scope_destroy(checker.scope);
    checker.scope = nullptr;

    return !checker.has_error;
}

static void checker_init() {
    if (checker.scope) {
        scope_destroy(checker.scope);
    }
    scope_t *global = malloc(sizeof *global);
    *global = (scope_t){.kind = SCOPE_GLOBAL};
    checker.scope = global;
    checker.has_error = false;
}

static bool check_stmt(stmt_t *stmt) {
    switch (stmt->kind) {
        case STMT_FUN_DECL: {
            declare_fun(stmt->fun.identifier, stmt->fun.params, stmt->fun.return_type);

            push_fun_scope(stmt->fun.return_type);
            for (size_t i = 0; i < stmt->fun.params.size; i++) {
                fun_param_t param = stmt->fun.params.data[i];
                declare_var(param.identifier, param.type);
            }

            bool res = true;
            if (stmt->fun.body) {
                res = check_stmt(stmt->fun.body);
            }

            pop_scope(SCOPE_FUN);

            return res;
        }
        case STMT_VAR_DECL: {
            token_t identifier = stmt->var.identifier;
            type_t *declared = stmt->var.type;

            if (!stmt->var.value) {
                if (!declared) {
                    report_err(identifier.span, "variable needs a type or an initializer");
                    return false;
                }
                return declare_var(identifier, declared);
            }

            type_t *inferred = check_expr(stmt->var.value);
            if (!inferred) return false;

            if (declared && declared != inferred) {
                report_err(identifier.span, "declared type differs from assigned value");
                return false;
            }
            if (!declared) {
                stmt->var.type = inferred;
            }

            return declare_var(identifier, stmt->var.type);
        }
        case STMT_IF: {
            type_t *cond = check_expr(stmt->if_.then_condition);
            if (!cond) return false;
            if (cond != &ty_bool) {
                report_err(stmt->span, "condition must be a boolean");
                return false;
            }
            bool ok = check_stmt(stmt->if_.then_body);

            if (stmt->if_.else_body) {
                bool res = check_stmt(stmt->if_.else_body);
                ok = ok && res;
            }
            return ok;
        }
        case STMT_BLOCK: {
            bool ok = true;
            for (size_t i = 0; i < stmt->block.size; i++) {
                bool res = check_stmt(stmt->block.data[i]);
                ok = ok && res;
            }
            return ok;
        }
        case STMT_WHILE: {
            type_t *cond = check_expr(stmt->while_.condition);
            bool ok = cond != nullptr;
            if (cond && cond != &ty_bool) {
                report_err(stmt->while_.condition->span, "while condition must be a boolean");
                ok = false;
            }

            bool body_ok = check_stmt(stmt->while_.body);

            return ok && body_ok;
        }
        case STMT_FOR: {
            push_loop_scope();

            bool ok = true;
            if (stmt->for_.initializer) {
                ok = check_stmt(stmt->for_.initializer) && ok;
            }
            if (stmt->for_.condition) {
                type_t *cond = check_expr(stmt->for_.condition);
                if (!cond) {
                    ok = false;
                } else if (cond != &ty_bool) {
                    report_err(stmt->for_.condition->span, "for condition must be a boolean");
                    ok = false;
                }
            }
            if (stmt->for_.increment) {
                ok = check_expr(stmt->for_.increment) != nullptr && ok;
            }

            ok = check_stmt(stmt->for_.body) && ok;

            pop_scope(SCOPE_LOOP);
            return ok;
        }
        case STMT_BREAK: {
            if (!search_scope(SCOPE_LOOP)) {
                report_err(stmt->span, "break outside of a loop");
                return false;
            }
            return true;
        }
        case STMT_CONTINUE: {
            if (!search_scope(SCOPE_LOOP)) {
                report_err(stmt->span, "continue outside of a loop");
                return false;
            }
            return true;
        }
        case STMT_RETURN: {
            scope_t *fun_scope = search_scope(SCOPE_FUN);
            if (!fun_scope) {
                report_err(stmt->span, "returned outside of a function");
                return false;
            }

            type_t *return_type = stmt->return_.value ? check_expr(stmt->return_.value) : &ty_void;
            if (!return_type) return false;

            if (return_type != fun_scope->return_type) {
                report_err(
                    stmt->span,
                    "mismatched return type (expected " STR_FMT ", received " STR_FMT " instead)",
                    STR_ARG(fun_scope->return_type->lexeme),
                    STR_ARG(return_type->lexeme));
                return false;
            }
            return true;
        }
        case STMT_EXPR: {
            return check_expr(stmt->expr) != nullptr;
        }
        default: {
            report_err(stmt->span, "unrecognized statement");
            return false;
        }
    }
}

static type_t *check_expr_inner(expr_t *expr);
static type_t *check_binary(expr_t *expr);

static type_t *check_expr(expr_t *expr) {
    type_t *t = check_expr_inner(expr);
    expr->type = t;
    return t;
}

static type_t *check_expr_inner(expr_t *expr) {
    switch (expr->kind) {
        case EXPR_BINARY:
            return check_binary(expr);
        case EXPR_UNARY: {
            token_t op = expr->unary.op;

            type_t *val_type = check_expr(expr->unary.right);
            if (!val_type) return nullptr;

            switch (op.kind) {
                case TOKEN_BANG: {
                    if (val_type != &ty_bool) {
                        report_err(
                            expr->span,
                            "Bang (!) operator can only be "
                            "applied to boolean values");
                        return nullptr;
                    }
                    break;
                }
                case TOKEN_MINUS: {
                    if (!is_negatable(val_type)) {
                        report_err(
                            expr->span,
                            "Minus (-) operator can only be applied to "
                            "integer and float values");
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
            if (!fun_obj) {
                report_err(callee.span, "Called function is not defined");
                return nullptr;
            }
            if (fun_obj->fun.params.size != expr->call.args.size) {
                report_err(
                    callee.span,
                    "Wrong number of arguments (expected %zu, received %zu)",
                    fun_obj->fun.params.size,
                    expr->call.args.size);
                return nullptr;
            }

            bool had_err = false;
            for (size_t i = 0; i < expr->call.args.size; i++) {
                type_t *arg_type = check_expr(expr->call.args.data[i]);
                type_t *param_type = fun_obj->fun.params.data[i].type;
                if (!arg_type) {
                    had_err = true;
                    continue;
                }
                if (arg_type != param_type) {
                    report_err(
                        expr->call.args.data[i]->span,
                        "Mismatched types (expected " STR_FMT ", received " STR_FMT " instead)",
                        STR_ARG(param_type->lexeme),
                        STR_ARG(arg_type->lexeme));
                    had_err = true;
                }
            }
            if (had_err) return nullptr;

            return fun_obj->fun.return_type;
        }
        case EXPR_VAR: {
            obj_t *val_obj = get_obj(expr->var.lexeme);
            if (!val_obj) {
                report_err(expr->span, "Undeclared variable");
                return nullptr;
            }
            return val_obj->var.type;
        }
        case EXPR_PRIMARY: {
            switch (expr->primary.kind) {
                case TOKEN_INTEGER:
                    return &ty_int;
                case TOKEN_FLOAT:
                    return &ty_float;
                case TOKEN_TRUE:
                case TOKEN_FALSE:
                    return &ty_bool;
                case TOKEN_CHAR:
                    return &ty_char;
                case TOKEN_STRING:
                    return &ty_string;
                default:
                    break;
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
    if (!left_type || !right_type) {
        return nullptr;
    }
    if (left_type != right_type) {
        report_err(
            expr->span,
            "Mismatched operands types " STR_FMT " and " STR_FMT,
            STR_ARG(left_type->lexeme),
            STR_ARG(right_type->lexeme));
        return nullptr;
    }

    switch (expr->binary.op.kind) {
        case TOKEN_EQUAL: {
            if (expr->binary.left->kind != EXPR_VAR) {
                report_err(expr->binary.left->span, "Left side of assignment must be a variable");
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_AND:
        case TOKEN_OR: {
            if (left_type != &ty_bool) {
                report_err(expr->span, "Logical operators require boolean operands");
                return nullptr;
            }
            return &ty_bool;
        }
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
        case TOKEN_MINUS: {
            if (!is_computable(left_type)) {
                report_err(
                    expr->span, "Type " STR_FMT " cannot be computed", STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_PLUS: {
            if (!is_computable(left_type) && left_type != &ty_string) {
                report_err(
                    expr->span, "Type " STR_FMT " cannot be added", STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return left_type;
        }
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL: {
            if (!is_comparable(left_type)) {
                report_err(
                    expr->span, "Type " STR_FMT " cannot be compared", STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return &ty_bool;
        }
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL: {
            if (!is_comparable(left_type) && left_type != &ty_string && left_type != &ty_char &&
                left_type != &ty_bool) {
                report_err(
                    expr->span, "Type " STR_FMT " cannot be compared", STR_ARG(left_type->lexeme));
                return nullptr;
            }
            return &ty_bool;
        }
        default: {
            report_err(expr->span, "Unrecognized binary expression");
            return nullptr;
        }
    }
}
