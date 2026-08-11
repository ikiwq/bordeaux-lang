#include "codegen/quad.h"

#include <stdio.h>

static void print_quad(const quadruple_t*, size_t);
static void print_quad_arg(const quad_arg_t*);
static const char* quad_op_symbol(token_kind_t);

void print_quads(const quadruple_t *quads, const size_t quad_count) {
    for (size_t i = 0; i < quad_count; i++) {
        print_quad(&quads[i], i);
    }
}

static const char* quad_op_symbol(const token_kind_t op) {
    switch (op) {
        case QUAD_OP_ADD:      return "+";
        case QUAD_OP_SUBTRACT: return "-";
        case QUAD_OP_MULTIPLY: return "*";
        case QUAD_OP_DIVIDE:   return "/";
        case QUAD_OP_AND:      return "&&";
        case QUAD_OP_OR:       return "||";
        case QUAD_NEGATE:      return "-";
        case QUAD_OP_NOT:      return "!";
        case QUAD_OP_GOTO_LT:  return "<";
        case QUAD_OP_GOTO_LE:  return "<=";
        case QUAD_OP_GOTO_GT:  return ">";
        case QUAD_OP_GOTO_GE:  return ">=";
        case QUAD_OP_GOTO_EQ:  return "==";
        case QUAD_OP_GOTO_NE:  return "!=";
        default:                return "?";
    }
}

static void print_quad_arg(const quad_arg_t *arg) {
    if (arg == nullptr || arg->kind == QUAD_EMPTY) {
        printf("_");
        return;
    }

    switch (arg->kind) {
        case QUAD_NAME:
            printf("%s", arg->addr.name);
            break;

        case QUAD_REG:
            printf("t%zu", arg->addr.reg);
            break;

        case QUAD_CONSTANT:
            switch (arg->addr.constant.kind) {
                case QUAD_CONST_INT:
                    printf("%zu", arg->addr.constant._int);
                    break;
                case QUAD_CONST_CHAR:
                    printf("'%c'", arg->addr.constant._char);
                    break;
                case QUAD_CONST_STRING:
                    printf("\"%s\"", arg->addr.constant.string);
                    break;
                default:
                    printf("<unknown const kind>");
                    break;
            }
            break;

        default:
            printf("<unknown arg kind %d>", (int)arg->kind);
            break;
    }
}

static void print_quad(const quadruple_t *quad, const size_t index) {
    printf("%4zu: ", index);

    if (quad == nullptr) {
        printf("<null quad>\n");
        return;
    }

    switch (quad->op) {
        case QUAD_OP_LABEL:
            print_quad_arg(&quad->result);
            printf(":\n");
            break;

        case QUAD_OP_GOTO:
            printf("goto ");
            print_quad_arg(&quad->result);
            printf("\n");
            break;

        case QUAD_OP_GOTO_LT:
        case QUAD_OP_GOTO_LE:
        case QUAD_OP_GOTO_GT:
        case QUAD_OP_GOTO_GE:
        case QUAD_OP_GOTO_EQ:
        case QUAD_OP_GOTO_NE:
            printf("if ");
            print_quad_arg(&quad->arg1);
            printf(" %s ", quad_op_symbol(quad->op));
            print_quad_arg(&quad->arg2);
            printf(" goto ");
            print_quad_arg(&quad->result);
            printf("\n");
            break;

        case QUAD_OP_COPY:
            print_quad_arg(&quad->result);
            printf(" = ");
            print_quad_arg(&quad->arg1);
            printf("\n");
            break;

        case QUAD_OP_COPY_INDEXED_R:
            print_quad_arg(&quad->result);
            printf(" = ");
            print_quad_arg(&quad->arg1);
            printf("[");
            print_quad_arg(&quad->arg2);
            printf("]\n");
            break;

        case QUAD_OP_COPY_INDEXED_L:
            print_quad_arg(&quad->result);
            printf("[");
            print_quad_arg(&quad->arg2);
            printf("] = ");
            print_quad_arg(&quad->arg1);
            printf("\n");
            break;

        case QUAD_OP_COPY_DEREFED_R:
            print_quad_arg(&quad->result);
            printf(" = *");
            print_quad_arg(&quad->arg1);
            printf("\n");
            break;

        case QUAD_OP_COPY_DEREFED_L:
            printf("*");
            print_quad_arg(&quad->result);
            printf(" = ");
            print_quad_arg(&quad->arg1);
            printf("\n");
            break;

        case QUAD_NEGATE:
        case QUAD_OP_NOT:
            print_quad_arg(&quad->result);
            printf(" = %s", quad_op_symbol(quad->op));
            print_quad_arg(&quad->arg1);
            printf("\n");
            break;

        case QUAD_OP_CALL:
            print_quad_arg(&quad->result);
            printf(" = call ");
            print_quad_arg(&quad->arg1);
            printf(", ");
            print_quad_arg(&quad->arg2);
            printf("\n");
            break;

        case QUAD_OP_ADD:
        case QUAD_OP_SUBTRACT:
        case QUAD_OP_MULTIPLY:
        case QUAD_OP_DIVIDE:
        case QUAD_OP_AND:
        case QUAD_OP_OR:
            print_quad_arg(&quad->result);
            printf(" = ");
            print_quad_arg(&quad->arg1);
            printf(" %s ", quad_op_symbol(quad->op));
            print_quad_arg(&quad->arg2);
            printf("\n");
            break;

        default:
            printf("<unknown op %d>\n", (int)quad->op);
            break;
    }
}