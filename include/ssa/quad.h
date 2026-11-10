#ifndef BORDEAUX_QUAD_H
#define BORDEAUX_QUAD_H

#include "str.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_PARAM,
    OP_CONST,
    OP_ADD,
    OP_SUB
} op_kind_t;

typedef struct ssa_quad ssa_quad_t;
struct ssa_quad {
    uint8_t op;

    ssa_quad_t *arg1;
    ssa_quad_t *arg2;

    union {
        bool bool_;
        char char_;
        int64_t int_;
        double float_;
        str_t str;
    } const;
};

typedef struct {
    str_t key;
    ssa_quad_t *value;
} symbol_entry_t;

typedef struct {
    size_t id;

    symbol_entry_t **symbols;
    size_t symbol_cap;
    size_t symbol_size;
} block_t;

void put_symbol(block_t *block, str_t key, ssa_quad_t *value);

#endif // BORDEAUX_QUAD_H
