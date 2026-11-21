#include "transpiler/transpiler.h"
#include "file.h"
#include "parser/expr.h"
#include "parser/stmt.h"
#include "scanner/token.h"
#include "str.h"
#include "types.h"
#include <asm-generic/errno-base.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *out;
static size_t depth = 0;

void emit_statement(stmt_t *stmt);
void emit_expr(expr_t *expr);
void emit_primary(token_t token);
void emit_type(type_t *type);
void emit_indent();
void emit(const char *fmt, ...);

void transpile(stmt_vec_t stmts) {
    out = open_file("./main.c", "w");

    emit("#include \"runtime/runtime.h\"\n\n");

    for (size_t i = 0; i < stmts.size; i++) {
        emit_statement(stmts.data[i]);
    }

    fclose(out);
}

void emit_statement(stmt_t *stmt) {
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
        case STMT_FUN_DECL: {
            bool has_body = stmt->fun.body;
            if (!has_body) {
                emit("extern ");
            }

            emit_type(stmt->fun.return_type);
            emit(" " STR_FMT "(", STR_ARG(stmt->fun.identifier.lexeme));

            for (size_t i = 0; i < stmt->fun.params.size; i++) {
                fun_param_t param = stmt->fun.params.data[i];
                emit_type(param.type);
                emit(" " STR_FMT, STR_ARG(param.identifier.lexeme));
                if (i < stmt->fun.params.size - 1) {
                    emit(",");
                }
            }

            emit(")");

            if (has_body) {
                emit("{\n");
                emit_statement(stmt->fun.body);
                emit("}\n\n");
            } else {
                emit(";\n\n");
            }

            break;
        }
        case STMT_VAR_DECL: {
            emit_indent();
            emit_type(stmt->var.type);
            emit(" " STR_FMT, STR_ARG(stmt->var.identifier.lexeme));
            emit(" = ");
            emit_expr(stmt->var.value);
            emit(";\n");
            break;
        }
        case STMT_IF: {
            emit_indent();
            emit("if (");
            emit_expr(stmt->if_.then_condition);
            emit(") {\n");
            emit_statement(stmt->if_.then_body);

            if (stmt->if_.else_body) {
                emit_indent();
                emit("} else {\n");
                emit_statement(stmt->if_.else_body);
            }

            emit_indent();
            emit("}\n");
            break;
        }
        case STMT_BLOCK: {
            depth++;
            for (size_t i = 0; i < stmt->block.size; i++) {
                emit_statement(stmt->block.data[i]);
            }
            depth--;
            break;
        }
        case STMT_WHILE: {
            emit_indent();
            emit("while (");
            emit_expr(stmt->while_.condition);
            emit(") {\n");
            emit_statement(stmt->while_.body);
            emit_indent();
            emit("}\n");
            break;
        }
        case STMT_FOR: {
            emit_indent();
            emit("for (");

            if (stmt->for_.initializer) {
                stmt_t *init = stmt->for_.initializer;
                if (init->kind == STMT_VAR_DECL) {
                    emit_type(init->var.type);
                    emit(" " STR_FMT " = ", STR_ARG(init->var.identifier.lexeme));
                    emit_expr(init->var.value);
                } else {
                    emit_expr(init->expr);
                }
            }
            emit("; ");

            emit_expr(stmt->for_.condition);
            emit("; ");
            emit_expr(stmt->for_.increment);

            emit(") {\n");
            emit_statement(stmt->for_.body);
            emit_indent();
            emit("}\n");
            break;
        }
        case STMT_RETURN: {
            emit_indent();
            emit("return");
            if (stmt->return_.value) {
                emit(" ");
                emit_expr(stmt->return_.value);
            }
            emit(";\n");
            break;
        }
        case STMT_CONTINUE: {
            emit_indent();
            emit("continue;\n");
            break;
        }
        case STMT_BREAK: {
            emit_indent();
            emit("break;\n");
            break;
        }
        case STMT_EXPR: {
            emit_indent();
            emit_expr(stmt->expr);
            emit(";\n");
            break;
        }
        default: {
            break;
        }
    }
}

void emit_expr(expr_t *expr) {
    if (!expr) {
        return;
    }

    switch (expr->kind) {
        case EXPR_BINARY: {
            if (expr->binary.left->type == &ty_string) {
                emit("str_concat(");
                emit_expr(expr->binary.left);
                emit(", ");
                emit_expr(expr->binary.right);
                emit(")");
                return;
            }

            emit_expr(expr->binary.left);
            emit(" " STR_FMT " ", STR_ARG(expr->binary.op.lexeme));
            emit_expr(expr->binary.right);
            break;
        }
        case EXPR_PRIMARY: {
            emit_primary(expr->primary);
            break;
        }
        case EXPR_UNARY: {
            emit(STR_FMT, STR_ARG(expr->unary.op.lexeme));
            emit_expr(expr->unary.right);
            break;
        }
        case EXPR_CALL: {
            emit(STR_FMT "(", STR_ARG(expr->call.callee.lexeme));
            for (size_t i = 0; i < expr->call.args.size; i++) {
                emit_expr(expr->call.args.data[i]);
                if (i < expr->call.args.size - 1) {
                    emit(",");
                }
            }
            emit(")");

            break;
        }
        case EXPR_VAR: {
            emit(STR_FMT, STR_ARG(expr->var.lexeme));
            break;
        }
        default: {
            break;
        }
    }
}

void emit_primary(token_t token) {
    switch (token.kind) {
        case TOKEN_CHAR: {
            emit("'%c'", token.attr.char_val);
            break;
        }
        case TOKEN_TRUE: {
            emit("true");
            break;
        }
        case TOKEN_FALSE: {
            emit("false");
            break;
        }
        case TOKEN_INTEGER: {
            emit("%i", token.attr.num_val);
            break;
        }
        case TOKEN_FLOAT: {
            emit("%f", token.attr.float_val);
            break;
        }
        case TOKEN_STRING: {
            emit(
                "(str_t){ .ptr = \"" STR_FMT "\", .len = %zu }",
                STR_ARG(token.lexeme),
                token.lexeme.len);
        }
        default: {
            break;
        }
    }
}

void emit_type(type_t *type) {
    switch (type->kind) {
        case TY_INT: {
            emit("int");
            break;
        }
        case TY_FLOAT: {
            emit("float");
            break;
        }
        case TY_BOOL: {
            emit("bool");
            break;
        }
        case TY_CHAR: {
            emit("char");
            break;
        }
        case TY_STRING: {
            emit("str_t");
            break;
        }
        case TY_VOID: {
            emit("void");
            break;
        }
        default: {
            fprintf(stderr, "met unknown type during transpiling");
            exit(EINVAL);
        }
    }
}

void emit_indent() { fprintf(out, "%*s", (int)(depth * 4), ""); }

void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
}
