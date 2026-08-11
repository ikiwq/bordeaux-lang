#ifndef TRHEEAC_H
#define THREEAC_H

#include "analyzer/statement.h"
#include "parser/expr.h"
#include "scanner/token.h"

typedef enum {
    QUAD_OP_ADD,
    QUAD_OP_SUBTRACT,
    QUAD_OP_MULTIPLY,
    QUAD_OP_DIVIDE,
    QUAD_OP_AND,
    QUAD_OP_OR,

    QUAD_NEGATE,
    QUAD_OP_NOT,

    QUAD_OP_COPY,
    QUAD_OP_COPY_INDEXED_R,
    QUAD_OP_COPY_INDEXED_L,
    QUAD_OP_COPY_DEREFED_R,
    QUAD_OP_COPY_DEREFED_L,

    QUAD_OP_LABEL,
    QUAD_OP_GOTO,
    QUAD_OP_GOTO_LT,
    QUAD_OP_GOTO_LE,
    QUAD_OP_GOTO_GT,
    QUAD_OP_GOTO_GE,
    QUAD_OP_GOTO_EQ,
    QUAD_OP_GOTO_NE,

    QUAD_OP_CALL,

    QUAD_OP_UNKNOWN = -1
} quad_op_kind_t;

typedef enum {
    QUAD_CONST_INT,
    QUAD_CONST_CHAR,
    QUAD_CONST_STRING,
} quad_const_kind_t;

typedef struct {
    quad_const_kind_t kind;

    union {
        size_t _int;
        uint8_t _char;
        const char *string;
    };
} quad_const_t;

typedef enum {
    QUAD_EMPTY = 0,
    QUAD_NAME,
    QUAD_REG,
    QUAD_CONSTANT,

    QUAD_UNKNOWN = -1
} quad_arg_kind_t;

typedef struct {
    quad_arg_kind_t kind;

    union {
        const char *name;
        size_t reg;
        quad_const_t constant;
    } addr;
} quad_arg_t;

typedef struct {
    quad_op_kind_t op;

    quad_arg_t arg1;
    quad_arg_t arg2;
    quad_arg_t result;
} quadruple_t;

VEC_DEFINE_H(quadruple_t, quad)

typedef struct {
    fe_err_t *errs;
    size_t err_count;

    quadruple_t *quads;
    size_t quad_count;
} icg_result_t;

typedef struct {
    err_vec_t err_vec;
    quad_vec_t quad_vec;
} icg_t;

icg_result_t generate_quads(typed_stmt_t **t_stmts, size_t stmt_count);

void print_quads(const quadruple_t *quads, size_t quad_count);

#endif // THREEAC_H
