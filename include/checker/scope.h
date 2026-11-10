#ifndef BORDEAUX_SCOPE_H
#define BORDEAUX_SCOPE_H

#include "obj.h"
#include "str.h"

typedef struct {
    str_t key;
    obj_t value;
    bool active;
} obj_entry_t;

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_LOOP,
    SCOPE_FUN
} scope_kind_t;

typedef struct scope scope_t;
struct scope {
    scope_kind_t kind;
    scope_t *parent;
    type_t *return_type;

    obj_entry_t *entries;
    size_t capacity;
    size_t size;
};

void scope_destroy(scope_t *scope);

void push_fun_scope(type_t *return_type);
void push_loop_scope();

scope_t *search_scope(scope_kind_t expected);
void pop_scope(scope_kind_t expected);

obj_t *get_obj(str_t lexeme);

bool declare_var(token_t identifier, type_t *type);
bool define_var(token_t identifier);

bool declare_fun(token_t identifier, fun_param_t **params, size_t param_count, type_t *return_type);

#endif // BORDEAUX_SCOPE_H
