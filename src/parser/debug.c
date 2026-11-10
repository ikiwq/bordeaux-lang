#include "parser/debug.h"

#include <stdio.h>
#include <inttypes.h>

#include "str.h"

static void indent(const size_t level) {
    printf("%*s", (int)(level * 2), "");
}

static void print_label(const char *label, const size_t level) {
    indent(level);
    printf("%s:\n", label);
}

static void print_type(const type_t *type) {
    if (type == nullptr) {
        printf("<inferred>");
        return;
    }
    printf(STR_FMT, STR_ARG(type->lexeme));
}

void print_stmts(stmt_t **stmts, const size_t stmt_count) {
    for (size_t i = 0; i < stmt_count; i++) {
        print_stmt(stmts[i], 0);
    }
}

void print_expr(const expr_t *expr, const size_t level) {
    indent(level);
    if (expr == nullptr) {
        printf("<null expr>\n");
        return;
    }

    switch (expr->kind) {
        case EXPR_VAR: {
            printf("Var: " STR_FMT "\n", STR_ARG(expr->var.lexeme));
            break;
        }

        case EXPR_PRIMARY: {
            const token_t token = expr->primary;
            switch (token.kind) {
                case TOKEN_INTEGER: printf("Int: %" PRIi64 "\n", token.attr.num_val); break;
                case TOKEN_FLOAT:   printf("Float: %g\n", token.attr.float_val); break;
                case TOKEN_STRING:  printf("String: \"" STR_FMT "\"\n", STR_ARG(token.lexeme)); break;
                case TOKEN_CHAR:    printf("Char: '%c'\n", token.attr.char_val); break;
                case TOKEN_TRUE:
                case TOKEN_FALSE:   printf("Bool: %s\n", token.attr.bool_val ? "true" : "false"); break;
                default:            printf("Primary: " STR_FMT "\n", STR_ARG(token.lexeme)); break;
            }
            break;
        }

        case EXPR_BINARY: {
            printf("Binary: " STR_FMT "\n", STR_ARG(expr->binary.op.lexeme));
            print_expr(expr->binary.left, level + 1);
            print_expr(expr->binary.right, level + 1);
            break;
        }

        case EXPR_UNARY: {
            printf("Unary: " STR_FMT "\n", STR_ARG(expr->unary.op.lexeme));
            print_expr(expr->unary.right, level + 1);
            break;
        }

        case EXPR_CALL: {
            printf("Call: " STR_FMT "\n", STR_ARG(expr->call.callee.lexeme));
            for (size_t i = 0; i < expr->call.arg_count; i++) {
                print_expr(expr->call.args[i], level + 1);
            }
            break;
        }

        default: {
            printf("<unknown expr>\n");
            break;
        }
    }
}

void print_stmt(const stmt_t *stmt, const size_t level) {
    indent(level);
    if (stmt == nullptr) {
        printf("<null stmt>\n");
        return;
    }

    switch (stmt->kind) {
        case STMT_FUN_DECL: {
            printf("Fun: " STR_FMT " -> ", STR_ARG(stmt->fun.identifier.lexeme));
            print_type(stmt->fun.return_type);
            printf("\n");
            for (size_t i = 0; i < stmt->fun.param_count; i++) {
                const fun_param_t *param = stmt->fun.params[i];
                indent(level + 1);
                printf("Param: " STR_FMT ": ", STR_ARG(param->identifier.lexeme));
                print_type(param->type);
                printf("\n");
            }
            print_stmt(stmt->fun.body, level + 1);
            break;
        }

        case STMT_VAR_DECL: {
            printf("Let: " STR_FMT ": ", STR_ARG(stmt->var.identifier.lexeme));
            print_type(stmt->var.type);
            printf("\n");
            print_expr(stmt->var.value, level + 1);
            break;
        }

        case STMT_IF: {
            printf("If\n");
            print_label("Condition", level + 1);
            print_expr(stmt->if_.then_condition, level + 2);
            print_label("Then", level + 1);
            print_stmt(stmt->if_.then_body, level + 2);
            if (stmt->if_.else_body) {
                print_label("Else", level + 1);
                print_stmt(stmt->if_.else_body, level + 2);
            }
            break;
        }

        case STMT_FOR: {
            printf("For\n");
            if (stmt->for_.initializer) {
                print_label("Initializer", level + 1);
                print_stmt(stmt->for_.initializer, level + 2);
            }
            if (stmt->for_.condition) {
                print_label("Condition", level + 1);
                print_expr(stmt->for_.condition, level + 2);
            }
            if (stmt->for_.increment) {
                print_label("Increment", level + 1);
                print_expr(stmt->for_.increment, level + 2);
            }
            print_label("Body", level + 1);
            print_stmt(stmt->for_.body, level + 2);
            break;
        }

        case STMT_WHILE: {
            printf("While\n");
            print_label("Condition", level + 1);
            print_expr(stmt->while_.condition, level + 2);
            print_label("Body", level + 1);
            print_stmt(stmt->while_.body, level + 2);
            break;
        }

        case STMT_RETURN: {
            printf("Return\n");
            if (stmt->return_.value) {
                print_expr(stmt->return_.value, level + 1);
            }
            break;
        }

        case STMT_BLOCK: {
            printf("Block\n");
            for (size_t i = 0; i < stmt->block.stmt_count; i++) {
                print_stmt(stmt->block.stmts[i], level + 1);
            }
            break;
        }

        case STMT_BREAK: {
            printf("Break\n");
            break;
        }

        case STMT_CONTINUE: {
            printf("Continue\n");
            break;
        }

        case STMT_EXPR: {
            printf("Expr\n");
            print_expr(stmt->expr, level + 1);
            break;
        }

        default: {
            printf("<unknown stmt>\n");
            break;
        }
    }
}
