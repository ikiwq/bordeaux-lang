#include "parser/parser.h"

#include <stdio.h>
#include <inttypes.h>

static void print_statement(const stmt_t*, size_t);
static void print_expression(const expr_t*, size_t);

void print_statements(stmt_t** statements, const size_t statement_count) {
    for (size_t i = 0; i < statement_count; i++) {
        print_statement(statements[i], 0);
    }
}

static void indent(const size_t level) {
    printf("%*s", (int)(level * 2), "");
}

static void print_expression(const expr_t *expr, const size_t level) {
    indent(level);
    if (expr == nullptr) {
        printf("<null expr>\n");
        return;
    }

    switch (expr->kind) {
        case EXPR_IDENTIFIER:
            printf("Name: %.*s\n", (int)expr->as.identifier.span.length, expr->as.identifier.span.src);
            break;

        case EXPR_LITERAL:
            switch (expr->as.literal.kind) {
                case LIT_INTEGER:   printf("Int: %" PRIu64 "\n", expr->as.literal._int); break;
                case LIT_FLOAT:     printf("Float: %g\n", expr->as.literal._float); break;
                case LIT_STRING:    printf("String: \"%.*s\"\n", (int)expr->as.literal.string.length, expr->as.literal.string.start); break;
                case LIT_CHAR:      printf("Char: U+%04X\n", expr->as.literal._char); break;
                case LIT_BOOL:      printf("Bool: %s\n", expr->as.literal._bool ? "true" : "false"); break;
                default:            printf("Literal: <unknown kind>\n"); break;
            }
            printf("\n");
            break;

        case EXPR_ARR_LITERAL:
            printf("ArrayLiteral (%zu)\n", expr->as.array_literal.element_count);
            printf("\n");
            for (size_t i = 0; i < expr->as.array_literal.element_count; i++) {
                print_expression(expr->as.array_literal.elements[i], level + 1);
            }
            break;

        case EXPR_INDEX:
            printf("Index\n");
            indent(level + 1); printf("array:\n");
            print_expression(expr->as.index.array, level + 2);
            indent(level + 1); printf("index:\n");
            print_expression(expr->as.index.index, level + 2);
            break;

        case EXPR_UNARY:
            printf("Unary '%.*s'\n", (int)expr->as.unary.op.span.length, expr->as.unary.op.span.src);
            print_expression(expr->as.unary.right, level + 1);
            break;

        case EXPR_GROUP:
            printf("Group\n");
            print_expression(expr->as.group.inner, level + 1);
            break;

        case EXPR_BINARY:
            printf("Binary '%.*s'\n", (int)expr->as.binary.op.span.length, expr->as.binary.op.span.src);
            print_expression(expr->as.binary.left, level + 1);
            print_expression(expr->as.binary.right, level + 1);
            break;

        case EXPR_CALL:
            printf("Call (%zu args)\n", expr->as.call.args_count);
            indent(level + 1); printf("callee:\n");
            print_expression(expr->as.call.callee, level + 2);
            for (size_t i = 0; i < expr->as.call.args_count; i++) {
                indent(level + 1); printf("arg %zu:\n", i);
                print_expression(expr->as.call.args[i], level + 2);
            }
            break;

        case EXPR_TYPE_NAME:
            printf("TypeName: %.*s\n",
                   (int)expr->as.type_name.span.length,
                   expr->as.type_name.span.src);
            break;

        case EXPR_ARR_TYPE:
            printf("ArrayType\n");
            indent(level + 1); printf("len:\n");
            print_expression(expr->as.array_type.len, level + 2);
            indent(level + 1); printf("element:\n");
            print_expression(expr->as.array_type.element, level + 2);
            break;

        case EXPR_SLICE_TYPE:
            printf("SliceType\n");
            print_expression(expr->as.slice_type.element, level + 1);
            break;

        case EXPR_REFERENCE_TYPE:
            printf("ReferenceType\n");
            print_expression(expr->as.reference_type, level + 1);
            break;

        case EXPR_POINTER_TYPE:
            printf("PointerType\n");
            print_expression(expr->as.pointer_type, level + 1);
            break;

        default:
            printf("<unknown expr kind %d>\n", (int)expr->kind);
            break;
    }
}

static void print_statement(const stmt_t *stmt, const size_t level) {
    indent(level);
    if (stmt == nullptr) {
        printf("<null stmt>\n");
        return;
    }

    switch (stmt->kind) {
        case STMT_VAR_DECLARATION:
            printf("VarDecl '%.*s'\n", (int)stmt->as.var_decl.identifier.span.length, stmt->as.var_decl.identifier.span.src);
            if (stmt->as.var_decl.type) {
                indent(level + 1); printf("type:\n");
                print_expression(stmt->as.var_decl.type, level + 2);
            } else {
                indent(level + 1); printf("type: <inferred>\n");
            }
            indent(level + 1); printf("init:\n");
            print_expression(stmt->as.var_decl.initializer, level + 2);
            break;

        case STMT_FUN_DECLARATION:
            printf("FunDecl '%.*s' (%zu params)\n",
                    (int)stmt->as.fun_decl.identifier.span.length,
                    stmt->as.fun_decl.identifier.span.src,
                    stmt->as.fun_decl.params_count);
            for (size_t i = 0; i < stmt->as.fun_decl.params_count; i++) {
                const fun_parameter_t p = stmt->as.fun_decl.params[i];
                indent(level + 1); printf("param '%.*s':\n", (int)p.name.span.length, p.name.span.src);
                print_expression(p.type, level + 2);
            }
            indent(level + 1); printf("returns:\n");
            if (stmt->as.fun_decl.return_type) {
                print_expression(stmt->as.fun_decl.return_type, level + 2);
            } else {
                indent(level + 2); printf("<void>\n");
            }
            indent(level + 1); printf("body:\n");
            print_statement(stmt->as.fun_decl.body, level + 2);
            break;

        case STMT_IF:
            printf("If\n");
            indent(level + 1); printf("cond:\n");
            print_expression(stmt->as._if.condition, level + 2);
            indent(level + 1); printf("then:\n");
            print_statement(stmt->as._if.then_branch, level + 2);
            if (stmt->as._if.else_branch) {
                indent(level + 1); printf("else:\n");
                print_statement(stmt->as._if.else_branch, level + 2);
            }
            break;

        case STMT_WHILE:
            printf("While\n");
            indent(level + 1); printf("cond:\n");
            print_expression(stmt->as._while.condition, level + 2);
            indent(level + 1); printf("body:\n");
            print_statement(stmt->as._while.body, level + 2);
            break;

        case STMT_FOR:
            printf("For\n");
            indent(level + 1); printf("init:\n");
            print_statement(stmt->as._for.init, level + 2);
            indent(level + 1); printf("cond:\n");
            print_expression(stmt->as._for.condition, level + 2);
            indent(level + 1); printf("incr:\n");
            print_expression(stmt->as._for.increment, level + 2);
            indent(level + 1); printf("body:\n");
            print_statement(stmt->as._for.body, level + 2);
            break;

        case STMT_RETURN:
            printf("Return\n");
            if (stmt->as._return.value) {
                print_expression(stmt->as._return.value, level + 1);
            } else {
                indent(level + 1); printf("<void>\n");
            }
            break;

        case STMT_BLOCK:
            printf("Block (%zu)\n", stmt->as.block.stmt_count);
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                print_statement(stmt->as.block.stmts[i], level + 1);
            }
            break;

        case STMT_BREAK:    printf("Break\n"); break;
        case STMT_CONTINUE: printf("Continue\n"); break;

        case STMT_EXPR:
            printf("ExprStmt\n");
            print_expression(stmt->as.expr, level + 1);
            break;

        default:
            printf("<Unknown statement kind%d>\n", (int)stmt->kind);
            break;
    }
}
