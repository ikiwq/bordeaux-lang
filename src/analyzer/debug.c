#include "analyzer/statement.h"

#include <inttypes.h>

static void print_typed_statement(const typed_stmt_t *, size_t);
static void print_typed_expression(const typed_expr_t *, size_t);

void print_typed_stmts(typed_stmt_t **stmts, const size_t stmt_count) {
    for (size_t i = 0; i < stmt_count; i++) {
        print_typed_statement(stmts[i], 0);
    }
}

static void indent(const size_t level) {
    printf("%*s", (int)(level * 2), "");
}

static void print_type(const type_t *type) {
    if (type == nullptr) {
        printf("<null type>");
        return;
    }

    printf("%s", type->lexeme);
}

static void print_type_suffix(const type_t *type) {
    printf(" : ");
    print_type(type);
}

static void print_type_line(const type_t *type, const size_t level) {
    indent(level);
    print_type(type);
    printf("\n");
}

static void print_typed_expression(const typed_expr_t *expr, const size_t level) {
    indent(level);
    if (expr == nullptr) {
        printf("<null expr>\n");
        return;
    }

    switch (expr->kind) {
        case TY_EXPR_IDENTIFIER:
            printf("Name: %.*s",
                (int)expr->as.identifier.span.length, expr->as.identifier.span.src);
            print_type_suffix(expr->type);
            printf("\n");
            break;

        case TY_EXPR_LITERAL:
            switch (expr->as.literal.kind) {
                case LIT_INTEGER: printf("Int: %" PRIu64, expr->as.literal._int); break;
                case LIT_FLOAT:   printf("Float: %g", expr->as.literal._float); break;
                case LIT_STRING:  printf("String: \"%.*s\"",
                                      (int)expr->as.literal.string.length,
                                      expr->as.literal.string.start); break;
                case LIT_CHAR:    printf("Char: U+%04X", expr->as.literal._char); break;
                case LIT_BOOL:    printf("Bool: %s", expr->as.literal._bool ? "true" : "false"); break;
                default:          printf("Literal: <unknown kind>"); break;
            }
            print_type_suffix(expr->type);
            printf("\n");
            break;

        // case TY_EXPR_ARR_LITERAL:
        //     printf("ArrayLiteral (%zu)", expr->as.array_literal.element_count);
        //     print_type_suffix(expr->type);
        //     printf("\n");
        //     for (size_t i = 0; i < expr->as.array_literal.element_count; i++) {
        //         print_typed_expression(expr->as.array_literal.elements[i], level + 1);
        //     }
        //     break;

        case TY_EXPR_INDEX:
            printf("Index");
            print_type_suffix(expr->type);
            printf("\n");
            indent(level + 1); printf("array:\n");
            print_typed_expression(expr->as.index.array, level + 2);
            indent(level + 1); printf("index:\n");
            print_typed_expression(expr->as.index.index, level + 2);
            break;

        case TY_EXPR_UNARY:
            printf("Unary '%.*s'",
                (int)expr->as.unary.op.span.length, expr->as.unary.op.span.src);
            print_type_suffix(expr->type);
            printf("\n");
            print_typed_expression(expr->as.unary.right, level + 1);
            break;

        case TY_EXPR_GROUP:
            printf("Group");
            print_type_suffix(expr->type);
            printf("\n");
            print_typed_expression(expr->as.group.inner, level + 1);
            break;

        case TY_EXPR_BINARY:
            printf("Binary '%.*s'",
                (int)expr->as.binary.op.span.length, expr->as.binary.op.span.src);
            print_type_suffix(expr->type);
            printf("\n");
            print_typed_expression(expr->as.binary.left, level + 1);
            print_typed_expression(expr->as.binary.right, level + 1);
            break;

        case TY_EXPR_CALL:
            printf("Call (%zu args)", expr->as.call.args_count);
            print_type_suffix(expr->type);
            printf("\n");
            indent(level + 1); printf("callee:\n");
            print_typed_expression(expr->as.call.callee, level + 2);
            for (size_t i = 0; i < expr->as.call.args_count; i++) {
                indent(level + 1); printf("arg %zu:\n", i);
                print_typed_expression(expr->as.call.args[i], level + 2);
            }
            break;

        default:
            printf("<unknown expr kind %d>\n", (int)expr->kind);
            break;
    }
}

static void print_typed_statement(const typed_stmt_t *stmt, const size_t level) {
    indent(level);
    if (stmt == nullptr) {
        printf("<null stmt>\n");
        return;
    }

    switch (stmt->kind) {
        case STMT_VAR_DECLARATION:
            printf("VarDecl '%.*s'\n",
                (int)stmt->as.var_decl.identifier.span.length,
                stmt->as.var_decl.identifier.span.src);
            indent(level + 1); printf("type:\n");
            if (stmt->as.var_decl.type != nullptr) {
                print_type_line(stmt->as.var_decl.type, level + 2);
            } else {
                indent(level + 2); printf("<inferred>\n");
            }
            indent(level + 1); printf("init:\n");
            print_typed_expression(stmt->as.var_decl.initializer, level + 2);
            break;

        case STMT_FUN_DECLARATION:
            printf("FunDecl '%.*s' (%zu params)\n",
                (int)stmt->as.fun_decl.identifier.span.length,
                stmt->as.fun_decl.identifier.span.src,
                stmt->as.fun_decl.param_count);
            for (size_t i = 0; i < stmt->as.fun_decl.param_count; i++) {
                const typed_fun_param_t p = stmt->as.fun_decl.params[i];
                indent(level + 1);
                printf("param '%.*s':\n", (int)p.name.span.length, p.name.span.src);
                print_type_line(p.type, level + 2);
            }
            indent(level + 1); printf("returns:\n");
            if (stmt->as.fun_decl.return_type != nullptr) {
                print_type_line(stmt->as.fun_decl.return_type, level + 2);
            } else {
                indent(level + 2); printf("<void>\n");
            }
            indent(level + 1); printf("body:\n");
            print_typed_statement(stmt->as.fun_decl.body, level + 2);
            break;

        case STMT_IF:
            printf("If\n");
            indent(level + 1); printf("cond:\n");
            print_typed_expression(stmt->as._if.condition, level + 2);
            indent(level + 1); printf("then:\n");
            print_typed_statement(stmt->as._if.then_branch, level + 2);
            if (stmt->as._if.else_branch != nullptr) {
                indent(level + 1); printf("else:\n");
                print_typed_statement(stmt->as._if.else_branch, level + 2);
            }
            break;

        case STMT_WHILE:
            printf("While\n");
            indent(level + 1); printf("cond:\n");
            print_typed_expression(stmt->as._while.condition, level + 2);
            indent(level + 1); printf("body:\n");
            print_typed_statement(stmt->as._while.body, level + 2);
            break;

        case STMT_FOR:
            printf("For\n");
            indent(level + 1); printf("init:\n");
            print_typed_statement(stmt->as._for.init, level + 2);
            indent(level + 1); printf("cond:\n");
            print_typed_expression(stmt->as._for.condition, level + 2);
            indent(level + 1); printf("incr:\n");
            print_typed_expression(stmt->as._for.increment, level + 2);
            indent(level + 1); printf("body:\n");
            print_typed_statement(stmt->as._for.body, level + 2);
            break;

        case STMT_RETURN:
            printf("Return\n");
            if (stmt->as._return.value != nullptr) {
                print_typed_expression(stmt->as._return.value, level + 1);
            } else {
                indent(level + 1); printf("<void>\n");
            }
            break;

        case STMT_BLOCK:
            printf("Block (%zu)\n", stmt->as.block.stmt_count);
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                print_typed_statement(stmt->as.block.stmts[i], level + 1);
            }
            break;

        case STMT_BREAK:    printf("Break\n"); break;
        case STMT_CONTINUE: printf("Continue\n"); break;

        case STMT_EXPR:
            printf("ExprStmt\n");
            print_typed_expression(stmt->as.expr, level + 1);
            break;

        default:
            printf("<Unknown statement kind %d>\n", (int)stmt->kind);
            break;
    }
}