#ifndef ANALYZER_H
#define ANALYZER_H

#include <stddef.h>

#include "analyzer/scope.h"
#include "analyzer/statement.h"

#define STRMAP_TYPE type_t
#define STRMAP_NAME type
#include "dsa/strmap.h"

typedef struct {
    tstmt_avec_t *tstmt_avec;
    err_avec_t *err_avec; 

    arena_t *arena;
} analyzer_result_t;

typedef struct {
    err_avec_t *err_avec;
    type_map_t *type_map;
    ascope_t *scope;

    arena_t *arena;
} analyzer_t;

analyzer_result_t analyze(stmt_avec_t *stmt_avec);

#endif // ANALYZER_H
