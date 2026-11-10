#include "checker/scope.h"

#include <complex.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "checker/checker.h"
#include "checker/obj.h"
#include "common.h"
#include "mem/mem.h"
#include "parser/stmt.h"
#include "scanner/token.h"
#include "str.h"

static void put_entry(str_t key, obj_entry_t value);
static void resize_entries();
static obj_entry_t *find_entry(obj_entry_t *entries, size_t capacity, str_t key);

void scope_destroy(scope_t *scope) {
    free(scope->entries);
    free(scope);
}

void push_fun_scope(type_t *return_type) {
    scope_t *scope = malloc(sizeof *scope);
    *scope = (scope_t) {
        .kind = SCOPE_FUN,
        .parent = checker.scope,
        .return_type = return_type,
    };
    checker.scope = scope;
}

void push_loop_scope() {
    scope_t *scope = malloc(sizeof *scope);
    *scope = (scope_t) {
        .kind = SCOPE_LOOP,
        .parent = checker.scope,
    };
    checker.scope = scope;
}

scope_t *search_scope(scope_kind_t expected) {
    for(scope_t *scope = checker.scope; scope != nullptr; scope = scope->parent) {
        if(expected == SCOPE_LOOP && scope->kind == SCOPE_FUN) break;
        if(scope->kind == expected) return scope;
    }
    return nullptr;
}

void pop_scope(scope_kind_t expected) {
    if(!checker.scope || checker.scope->kind != expected) {
        fprintf(stderr, "Unexpected scope pop");
        exit(1);
    }
    scope_t *old = checker.scope;
    checker.scope = checker.scope->parent;

    scope_destroy(old);
}

obj_t *get_obj(str_t lexeme) {
    for(scope_t *scope = checker.scope; scope != nullptr; scope = scope->parent) {
        if(scope->capacity > 0) {
            obj_entry_t *entry = find_entry(scope->entries, scope->capacity, lexeme);
            if(entry->active) return &entry->value;
        }
        if(scope->kind == SCOPE_FUN && scope->parent->kind != SCOPE_GLOBAL) break;
    }
    return nullptr;
}

bool declare_var(token_t identifier, type_t *type) {
    resize_entries();

    scope_t *current_scope = checker.scope;
    obj_entry_t *entry = find_entry(current_scope->entries, current_scope->capacity, identifier.lexeme);
    if(entry->active) {
        report_err(identifier.span, "redeclared variable '" STR_FMT "'", STR_ARG(identifier.lexeme));
        return false;
    }

    *entry = (obj_entry_t) {
        .key = identifier.lexeme,
        .value = (obj_t) {
            .lexeme = identifier.lexeme,
            .obj_kind = OBJ_VAR,
            .var.type = type
        },
        .active = true,
    };
    current_scope->size++;

    return true;
}

bool define_var(token_t identifier) {
    obj_t *obj = get_obj(identifier.lexeme);
    if(!obj) {
        report_err(identifier.span, "cannot define undeclared variable '" STR_FMT "'", STR_ARG(identifier.lexeme));
        checker.has_error = true;
        return false;
    }
    if(obj->obj_kind != OBJ_VAR) {
        report_err(identifier.span, "cannot define non variable object'" STR_FMT "'", STR_ARG(identifier.lexeme));
        return false;
    }
    obj->defined = true;
    return true;
}

bool declare_fun(token_t identifier, fun_param_t **params, size_t param_count, type_t *return_type) {
    resize_entries();

    scope_t *current_scope = checker.scope;
    obj_entry_t *entry = find_entry(current_scope->entries, current_scope->capacity, identifier.lexeme);
    if(entry->active) {
        report_err(identifier.span, "redeclared function'" STR_FMT "'", STR_ARG(identifier.lexeme));
        return false;
    }

    *entry = (obj_entry_t) {
        .key = identifier.lexeme,
        .value = (obj_t) {
            .lexeme = identifier.lexeme,
            .obj_kind = OBJ_FUN,
            .fun = {
                .params = params,
                .param_count = param_count,
                .return_type = return_type
            }
        },
        .active = true
    };
    current_scope->size++;

    return true;
}

#define MAX_ENTRIES_LOAD 0.75

static void resize_entries() {
    scope_t *current_scope = checker.scope;
    if (current_scope->size < current_scope->capacity * MAX_ENTRIES_LOAD) return;

    size_t new_capacity = CAPACITY_GROW(current_scope->capacity);
    obj_entry_t *new_entries = calloc(new_capacity, sizeof(obj_entry_t));

    for(size_t i = 0; i < current_scope->capacity; i++) {
        obj_entry_t old_entry = current_scope->entries[i];
        if(!old_entry.active) continue;

        obj_entry_t *entry = find_entry(new_entries, new_capacity, old_entry.key);
        *entry = old_entry;
    }

    free(current_scope->entries);

    current_scope->capacity = new_capacity;
    current_scope->entries = new_entries;
}

static obj_entry_t *find_entry(obj_entry_t *entries, size_t cap, str_t key) {
    size_t index = str_hash(key) % cap;
    for(size_t i = 0; i < cap; i++) {
        obj_entry_t *entry = &entries[index];
        if(!entry->active || str_eq(entry->key, key)) return entry;
        index = (index + 1) % cap;
    }
    fprintf(stderr, "find_entry: table full\n");
    exit(1);
}
