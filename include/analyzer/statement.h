#ifndef ANALYZER_STATEMENT_H
#define ANALYZER_STATEMENT_H

#include "parser/parser.h"
#include "analyzer/expr.h"

typedef struct tstmt tstmt_t;

#define AVEC_TYPE tstmt_t*
#define AVEC_NAME tstmt
#include "dsa/arena_vec.h"

struct tstmt {
    stmt_kind_t kind;
    span_t span;

    union {
        struct {
            texpr_t *condition;
            tstmt_t *then_branch;
            tstmt_t *else_branch;
        } _if;

        struct {
            texpr_t *condition;
            tstmt_t *body;
        } _while;

        struct {
            tstmt_t *init;
            texpr_t *condition;
            texpr_t *increment;
            tstmt_t *body;
        } _for;

        struct {
            token_t identifier;
            texpr_t *initializer;
            type_t *type;
        } var_decl;

        struct {
            token_t identifier;
            tfunparam_avec_t *params;
            type_t *return_type;
            tstmt_t *body;
        } fun_decl;

        struct {
            texpr_t *value;
        } _return;

        tstmt_avec_t *block;

        texpr_t *expr;
    } as;
};

void print_tstmts(tstmt_avec_t*);

#endif // ANALYZER_STATEMENT_H

